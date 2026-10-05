"""Keep the backlog labels true when an issue is closed.

Called by .github/workflows/backlog.yml with the number of the issue that was just closed:
  - every issue it was blocking, and that no open issue blocks any more, goes from `statut:bloqué` to `statut:prêt`;
  - its parent is closed when all the parent's sub-issues are closed, then the parent's parent, and so on.

Needs the `gh` CLI with a token that can write issues (GH_TOKEN) and the repository in GH_REPO.
"""

from __future__ import annotations

import json
import os
import subprocess
import sys

BLOCKED, READY = "statut:bloqué", "statut:prêt"
QUERY = """
query($owner: String!, $name: String!, $number: Int!) {
  repository(owner: $owner, name: $name) {
    issue(number: $number) {
      blocking(first: 100) { nodes { number state blockedBy(first: 100) { nodes { state } } } }
      parent { number state subIssues(first: 100) { nodes { state } } }
    }
  }
}
"""


def gh(*arguments: str) -> str:
    return subprocess.run(["gh", *arguments], check=True, capture_output=True, text=True).stdout


def issue(number: int) -> dict:
    owner, name = os.environ["GH_REPO"].split("/")
    out = gh("api", "graphql", "-f", f"query={QUERY}", "-f", f"owner={owner}", "-f", f"name={name}",
             "-F", f"number={number}")
    return json.loads(out)["data"]["repository"]["issue"]


def unblocked(closed: dict) -> list[int]:
    """Open issues that `closed` was blocking and that nothing open blocks any more."""
    return [node["number"] for node in closed["blocking"]["nodes"]
            if node["state"] == "OPEN" and all(b["state"] == "CLOSED" for b in node["blockedBy"]["nodes"])]


def finished_parent(closed: dict) -> int | None:
    """The parent of `closed` when it is open and all its sub-issues are closed."""
    parent = closed["parent"]
    if parent and parent["state"] == "OPEN" and all(s["state"] == "CLOSED" for s in parent["subIssues"]["nodes"]):
        return parent["number"]
    return None


def main() -> None:
    number = int(sys.argv[1])
    while True:
        closed = issue(number)
        for ready in unblocked(closed):
            gh("issue", "edit", str(ready), "--remove-label", BLOCKED, "--add-label", READY)
            print(f"#{ready} is ready")
        parent = finished_parent(closed)
        if parent is None:
            return
        gh("issue", "close", str(parent), "--comment", "Toutes les sous-issues sont fermées.")
        print(f"#{parent} closed")
        number = parent  # an issue closed by this token starts no workflow: follow the chain here


if __name__ == "__main__":
    main()
