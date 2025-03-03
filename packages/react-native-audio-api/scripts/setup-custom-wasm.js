#!/usr/bin/env node

const fs = require('fs');
const path = require('path');

const args = process.argv.slice(2);

/**
 * Checks if the given app configuration is an Expo project using the Metro bundler for web.
 *
 * @param {Object} appJson - The app configuration JSON object.
 * @returns {boolean} - Returns true if the project is an Expo project using the Metro bundler for web, otherwise false.
 */
function isExpoProjectWithExpo(appJson) {
  return (
    appJson.expo && appJson.expo.web && appJson.expo.web.bundler === 'metro'
  );
}

/**
 * Checks if the current project is an Expo project using the Metro bundler.
 *
 * This function attempts to read the `app.json` file from the current working directory
 * to determine if the project is an Expo project. If the project is an Expo project,
 * it further checks if the Metro bundler is being used.
 *
 * @returns {boolean} - Returns `true` if the project is an Expo project using the Metro bundler,
 *                      `false` if it is an Expo project without the Metro bundler, or if no Expo settings are found.
 */
function getIsExpoOnMetro() {
  try {
    const appJsonPath = path.join(process.cwd(), 'app.json');

    console.log(`> Reading Expo settings from (if any):\n ${appJsonPath}\n`);
    const appJson = require(appJsonPath);

    const isExpoAndMetro = isExpoProjectWithExpo(appJson);

    if (isExpoAndMetro) {
      console.log(`> Expo project with Metro bundler detected\n`);
      return true;
    } else {
      console.log(`> Expo project without Metro bundler detected\n`);
      return false;
    }
  } catch (error) {
    console.log(`No Expo settings found\n`);
    return false;
  }
}

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
function getOutputFilePath(isExpoAndMetro) {
  const publicFolder = path.resolve(
    args[0] || (isExpoAndMetro ? 'public' : 'web/static/js')
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
  const isExpoAndMetro = getIsExpoOnMetro();

  const inputPath = getInputFilePath();
  const outputPath = getOutputFilePath(isExpoAndMetro);

  copyFile(inputPath, outputPath);

  console.log(`> WebAssembly module setup complete\n`);
})();
