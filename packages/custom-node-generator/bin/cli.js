#!/usr/bin/env node

const { program } = require('commander');
const chalk = require('chalk');
const { generate } = require('../lib/generator');

program
  .name('audio-api-node-generator')
  .description('Generate template files for custom processor using react-native-audio-api.')
  .version('0.0.2');


program
  .command('create')
  .option('-o, --output <path>', 'output directory', '.')
  .action(async (options) => {
    try {
      const hasOutputFlag = process.argv.includes('-o') || process.argv.includes('--output');
      if (!hasOutputFlag) {
        console.log(chalk.yellow('Please specify an output directory with -o or --output:'));
        return;
      }
      await generate(options.output, 'basic');
    } catch (error) {
      console.error(chalk.red('❌ Error:'), error.message);
      process.exit(1);
    }
  });

program.parse();