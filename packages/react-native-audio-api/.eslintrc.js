/** @type {import('eslint').ESLint.ConfigData} */
module.exports = {
  extends: ['../../.eslintrc.js'],
  overrides: [
    {
      files: ['./src/**/*.{ts,tsx}'],
    },
  ],
  ignorePatterns: ['lib', 'src/web-core/custom/signalsmithStretch' ],
};
