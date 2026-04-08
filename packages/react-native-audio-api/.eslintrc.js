/** @type {import('eslint').ESLint.ConfigData} */
module.exports = {
  extends: ['../../.eslintrc.js'],
  overrides: [
    {
      files: ['./src/**/*.{ts,tsx}'],
    },
    {
      files: ['./tests/**/*.{ts,tsx}'],
      parserOptions: {
        project: './tsconfig.test.json',
        tsconfigRootDir: __dirname,
      },
    },
  ],
  ignorePatterns: ['lib', 'src/web-core/custom/signalsmithStretch'],
};
