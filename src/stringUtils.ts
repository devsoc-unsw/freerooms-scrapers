// Adapted from https://medium.com/@onlinemsr/javascript-string-format-the-best-3-ways-to-do-it-c6a12b4b94ed
export const formatString = (template: string, ...args: string[]) => {
  return template.replace(/{([0-9]+)}/g, match => {
    const idx = parseInt(match.substring(1, match.length - 1), 10);
    return args[idx] ?? match;
  });
}
