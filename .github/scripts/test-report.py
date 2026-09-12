"""Render existing host-test output for Actions and offline review; never rerun tests."""
import argparse
import html
from pathlib import Path


def cases(log):
    results = {}
    for line in log.splitlines():
        parts = line.split()
        if len(parts) == 4 and parts[0] == 'CASE' and parts[1] in ('PLAN', 'RUN', 'PASS'):
            results[(parts[2], parts[3])] = {
                'PLAN': 'NOT RUN', 'RUN': 'INCOMPLETE', 'PASS': 'PASS'
            }[parts[1]]
    return results


def render(log, outcome):
    results = cases(log)
    lines = ['### Host regression scenarios', '', f'Test step outcome: **{outcome}**.', '',
             'INCOMPLETE means the case started but did not finish (see the assertion/sanitizer log).',
             'NOT RUN means a planned case was not reached. Suites never started are absent.',
             'These are host checks, not a simulation of physical dashboard refresh.', '',
             '| Suite / display | Scenario | Result |', '| --- | --- | --- |']
    for (suite, case), status in results.items():
        label = case.removeprefix('test_').replace('_', ' ')
        lines.append(f'| {suite} | {label} | {status} |')
    if not results:
        lines.append('| — | No scenario results; check build/test log | NOT RUN |')
    lines += ['', 'Download **host-test-log** for the HTML report and complete console log.', '']
    markdown = '\n'.join(lines)
    rows = ''.join('<tr>' + ''.join(f'<td>{html.escape(v)}</td>' for v in
                   (suite, case.removeprefix('test_').replace('_', ' '), status)) + '</tr>'
                   for (suite, case), status in results.items())
    page = ('<!doctype html><html lang="en"><meta charset="utf-8">'
            '<title>BACCAble host regression report</title>'
            '<style>body{font:16px system-ui;margin:2rem}td,th{padding:.4rem;text-align:left;'
            'border-bottom:1px solid #aaa}pre{white-space:pre-wrap}</style>'
            '<h1>BACCAble host regression report</h1><p>Test step: ' + html.escape(outcome) +
            '</p><p>INCOMPLETE: started without completion. NOT RUN: planned but not reached. '
            'Suites never started are absent. Host checks do not simulate physical screen refresh.</p>'
            '<table><tr><th>Suite / display</th><th>Scenario</th><th>Result</th></tr>' + rows +
            '</table><details><summary>Full build and test log</summary><pre>' + html.escape(log) +
            '</pre></details></html>')
    return markdown, page


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--outcome', required=True)
    parser.add_argument('--summary', required=True)
    args = parser.parse_args()
    source = Path('host-tests.log')
    log = source.read_text(errors='replace') if source.exists() else 'No host-test log was produced.'
    markdown, page = render(log, args.outcome)
    Path('host-tests.html').write_text(page)
    with open(args.summary, 'a') as summary:
        summary.write(markdown)
