function* createAnnotations() {
  const gitDiffParser = require('gitdiff-parser');
  const gitDiffText = require("fs").readFileSync(
      "clang_format.diff").toString();
  const diffFiles = gitDiffParser.parse(gitDiffText);

  for (const diffFile of diffFiles) {
    for (const hunk of diffFile.hunks) {
      let annotation = {};
      annotation.path = diffFile.oldPath;
      annotation.annotation_level = 'failure';
      annotation.title = 'clang-format'
      annotation.start_line = hunk.oldStart;
      annotation.end_line = hunk.oldStart + hunk.oldLines;
      const suggestion = hunk.changes.filter(change => !change.isDelete).map(
          change => change.content).join("\n");
      annotation.message = `\`\`\`suggestion\n${suggestion}\n\`\`\``;
      yield annotation;
    }
  }
}

const annotations = [...createAnnotations()];

const {repo, owner} = context.repo;
await github.request(`POST /repos/${owner}/${repo}/check-runs`, {
  owner: owner,
  repo: repo,
  name: 'Clang format suggestions',
  head_sha: context.event.workflow_run.head_commit.id,
  status: 'completed',
  started_at: new Date().toISOString(),
  conclusion: 'failure',
  completed_at: new Date().toISOString(),
  output: {
    title: 'Test',
    summary: 'Test text',
    text: 'Test summary',
    annotations: annotations,
  },
})

console.log(annotations)
