"""Decisions of scripts/backlog_sync.py, on issue data shaped like the GitHub answer."""

from __future__ import annotations

import backlog_sync

DONE = {"state": "CLOSED", "stateReason": "COMPLETED"}
DROPPED = {"state": "CLOSED", "stateReason": "NOT_PLANNED"}
OPEN = {"state": "OPEN", "stateReason": None}


def connection(nodes: list[dict], total: int | None = None) -> dict:
    return {"totalCount": len(nodes) if total is None else total, "nodes": nodes}


def blocked(number: int, state: str, blockers: list[dict], total: int | None = None) -> dict:
    return {"number": number, "state": state, "blockedBy": connection(blockers, total)}


def parent(state: str, subs: list[dict], total: int | None = None) -> dict:
    return {"number": 5, "state": state, "subIssues": connection(subs, total)}


def closed_issue(blocking: list[dict], parent_issue: dict | None = None, reason: str = "COMPLETED") -> dict:
    return {"state": "CLOSED", "stateReason": reason, "blocking": {"nodes": blocking}, "parent": parent_issue}


def test_issue_with_no_open_blocker_becomes_ready() -> None:
    closed = closed_issue([blocked(10, "OPEN", [DONE, DONE]), blocked(11, "OPEN", [DONE, OPEN]),
                           blocked(12, "CLOSED", [DONE])])
    assert backlog_sync.unblocked(closed) == [10]


def test_abandoned_blocker_unblocks_nothing() -> None:
    assert backlog_sync.unblocked(closed_issue([blocked(10, "OPEN", [DONE])], reason="NOT_PLANNED")) == []
    assert backlog_sync.unblocked(closed_issue([blocked(10, "OPEN", [DONE, DROPPED])])) == []


def test_truncated_blocker_list_unblocks_nothing() -> None:
    assert backlog_sync.unblocked(closed_issue([blocked(10, "OPEN", [DONE, DONE], total=3)])) == []


def test_parent_closes_only_when_every_sub_issue_is_done() -> None:
    assert backlog_sync.finished_parent(closed_issue([], parent("OPEN", [DONE, DONE]))) == 5
    assert backlog_sync.finished_parent(closed_issue([], parent("OPEN", [DONE, OPEN]))) is None
    assert backlog_sync.finished_parent(closed_issue([], parent("OPEN", [DONE, DROPPED]))) is None
    assert backlog_sync.finished_parent(closed_issue([], parent("CLOSED", [DONE]))) is None
    assert backlog_sync.finished_parent(closed_issue([])) is None


def test_parent_stays_open_on_a_truncated_list_or_an_abandoned_child() -> None:
    assert backlog_sync.finished_parent(closed_issue([], parent("OPEN", [DONE] * 100, total=120))) is None
    assert backlog_sync.finished_parent(closed_issue([], parent("OPEN", [DONE]), reason="NOT_PLANNED")) is None
