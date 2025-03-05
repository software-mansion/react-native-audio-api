#!/usr/bin/env node

const fs = require('fs');
const path = require('path');

const args = process.argv.slice(2);

/**
 * Resolves and returns the absolute path to the SignalsmithStretch.mjs file.
 *
 * @returns {string} The absolute path to the SignalsmithStretch.mjs file.
 */
function getInputFilePath() {
  return path.resolve(
    __dirname,
    '../lib/module/web-core/custom/signalsmithStretch/SignalsmithStretch.mjs'
  );
}

/**
 * Generates the output file path for the WebAssembly module.
 *
 * @param {boolean} isExpoAndMetro - A flag indicating whether the environment is Expo and Metro.
 * @returns {string} The resolved output file path.
 */
function getOutputFilePath() {
  const publicFolder = path.resolve(
    args[0] || 'public'
  );

  const publicFile = './signalsmithStretch.mjs';
  const outputPath = path.join(publicFolder, publicFile);

  console.log(`> Output file path: ${outputPath}\n`);

  return outputPath;
}

/**
 * Copies a file from the input path to the output path.
 *
 * @param {string} inputPath - The path of the file to be copied.
 * @param {string} outputPath - The destination path where the file should be copied.
 */
function copyFile(inputPath, outputPath) {
  const data = fs.readFileSync(inputPath);
  fs.mkdirSync(path.dirname(outputPath), { recursive: true });
  fs.writeFileSync(outputPath, data);
}

(function main() {
  const inputPath = getInputFilePath();
  const outputPath = getOutputFilePath();

  copyFile(inputPath, outputPath);

  console.log(`> WebAssembly module setup complete\n`);
})();
