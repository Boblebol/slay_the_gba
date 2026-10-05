"""Decisions of scripts/backlog_sync.py, on issue data shaped like the GitHub answer."""

from __future__ import annotations

import backlog_sync


def node(number: int, state: str, blockers: list[str]) -> dict:
    return {"number": number, "state": state, "blockedBy": {"nodes": [{"state": s} for s in blockers]}}


def closed_issue(blocking: list[dict], parent: dict | None = None) -> dict:
    return {"blocking": {"nodes": blocking}, "parent": parent}


def test_issue_with_no_open_blocker_becomes_ready() -> None:
    closed = closed_issue([node(10, "OPEN", ["CLOSED", "CLOSED"]), node(11, "OPEN", ["CLOSED", "OPEN"]),
                           node(12, "CLOSED", ["CLOSED"])])
    assert backlog_sync.unblocked(closed) == [10]


def test_parent_closes_only_when_every_sub_issue_is_closed() -> None:
    def parent(state: str, subs: list[str]) -> dict:
        return {"number": 5, "state": state, "subIssues": {"nodes": [{"state": s} for s in subs]}}

    assert backlog_sync.finished_parent(closed_issue([], parent("OPEN", ["CLOSED", "CLOSED"]))) == 5
    assert backlog_sync.finished_parent(closed_issue([], parent("OPEN", ["CLOSED", "OPEN"]))) is None
    assert backlog_sync.finished_parent(closed_issue([], parent("CLOSED", ["CLOSED"]))) is None
    assert backlog_sync.finished_parent(closed_issue([])) is None
