"""Keep the backlog labels true when an issue is closed.

Called by .github/workflows/backlog.yml with the number of the issue that was just closed:
  - every issue it was blocking, and that no open issue blocks any more, goes from `statut:bloqué` to `statut:prêt`;
  - its parent is closed when all the parent's sub-issues are closed, then the parent's parent, and so on.

Nothing is decided from a partial list or from an abandoned issue: a list GitHub truncated is treated as
"still open", and an issue closed as "not planned" unblocks nothing and finishes no parent.

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
      state
      stateReason
      blocking(first: 100) {
        nodes { number state blockedBy(first: 100) { totalCount nodes { state stateReason } } }
      }
      parent { number state subIssues(first: 100) { totalCount nodes { state stateReason } } }
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


def done(node: dict) -> bool:
    """Closed because the work is finished, not because it was abandoned or is a duplicate."""
    return node["state"] == "CLOSED" and node.get("stateReason") == "COMPLETED"


def all_done(connection: dict) -> bool:
    """Every issue of a GitHub list is done. A list that was cut short (more than one page) is never complete."""
    nodes = connection["nodes"]
    return connection["totalCount"] == len(nodes) and all(done(node) for node in nodes)


def unblocked(closed: dict) -> list[int]:
    """Open issues that `closed` was blocking and that nothing unfinished blocks any more."""
    if not done(closed):
        return []
    return [node["number"] for node in closed["blocking"]["nodes"]
            if node["state"] == "OPEN" and all_done(node["blockedBy"])]


def finished_parent(closed: dict) -> int | None:
    """The parent of `closed` when it is open and all its sub-issues are done."""
    parent = closed["parent"]
    if done(closed) and parent and parent["state"] == "OPEN" and all_done(parent["subIssues"]):
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
