#!/usr/bin/env node
// UserPromptSubmit hook — repeats the user's prompt as additionalContext.
// Research suggests repeating the instruction improves instruction-following.

let data = '';
process.stdin.setEncoding('utf8');
process.stdin.on('data', chunk => { data += chunk; });
process.stdin.on('end', () => {
  try {
    const input = JSON.parse(data);
    const prompt = (input.prompt || '').trim();
    if (prompt.length >= 500) {
      process.stdout.write(JSON.stringify({ additionalContext: prompt }));
    }
  } catch (_) {
    // parsing failed — output nothing, hook is a no-op
  }
});
