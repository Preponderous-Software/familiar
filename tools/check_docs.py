#!/usr/bin/env python3
"""Check the design docs: relative links resolve, and every citation ID is defined.

Citation IDs look like [P1], [C2], [S11], [M2] or [G1]. Each one used in a
Markdown file must be defined as a table row "| <ID> |" in RESEARCH.md.
Relative links must point at a file that exists, and an "#anchor" must match a
heading in the target file. External links are not fetched.

Usage: python3 tools/check_docs.py  (from anywhere; exits 1 on any problem)
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CITE = re.compile(r"\[([PCSMG]\d+)\]")
DEFINED = re.compile(r"^\| ([PCSMG]\d+) \|", re.MULTILINE)
LINK = re.compile(r"\[[^\]]*\]\(([^)\s]+)\)")
HEADING = re.compile(r"^#{1,6} (.+)$", re.MULTILINE)
FENCE = re.compile(r"^```.*?^```", re.MULTILINE | re.DOTALL)


def slug(heading):
    """GitHub's heading anchor: lowercase, punctuation dropped, spaces to hyphens."""
    text = re.sub(r"[`*_]", "", heading.strip().lower())
    text = re.sub(r"[^\w\- ]", "", text)
    return text.replace(" ", "-")


def anchors(path):
    text = FENCE.sub("", path.read_text(encoding="utf-8"))
    return {slug(h) for h in HEADING.findall(text)}


def main():
    skip = lambda p: any(part == ".git" or part.startswith("build") or part == "_deps" for part in p.relative_to(ROOT).parts)
    docs = sorted(p for p in ROOT.rglob("*.md") if not skip(p))
    defined = set(DEFINED.findall((ROOT / "RESEARCH.md").read_text(encoding="utf-8")))
    problems = []
    cites = 0
    links = 0

    for doc in docs:
        rel = doc.relative_to(ROOT)
        text = FENCE.sub("", doc.read_text(encoding="utf-8"))

        for cid in CITE.findall(text):
            cites += 1
            if cid not in defined:
                problems.append(f"{rel}: citation [{cid}] is not defined in RESEARCH.md")

        for target in LINK.findall(text):
            if re.match(r"^[a-z][a-z0-9+.-]*:", target):
                continue  # external link or mailto:
            links += 1
            file_part, _, anchor = target.partition("#")
            dest = doc if not file_part else (doc.parent / file_part).resolve()
            if not dest.exists():
                problems.append(f"{rel}: link {target} points at a missing file")
                continue
            if anchor and dest.suffix == ".md" and anchor not in anchors(dest):
                problems.append(f"{rel}: link {target} has no matching heading")

    for p in problems:
        print(p)
    print(f"{len(docs)} docs, {cites} citations, {links} relative links, {len(problems)} problems")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
