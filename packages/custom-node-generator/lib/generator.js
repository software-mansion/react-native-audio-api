const fs = require('fs-extra');
const path = require('path');
const chalk = require('chalk');

class FileGenerator {
  constructor() {
    this.templatesDir = path.join(__dirname, '../templates');
  }

  async generate(options) {
    const { outputPath, template } = options;
    
    const templatePath = path.join(this.templatesDir, template);

    if (!await fs.pathExists(templatePath)) {
      throw new Error(`Template not found`);
    }
   
    await fs.ensureDir(outputPath);

    await this.copyTemplate(templatePath, outputPath);

    console.log(chalk.green(`Generated files in: ${outputPath}`));
  }

  async copyTemplate(templatePath, targetPath) {
    const files = await fs.readdir(templatePath);

    for (const file of files) {
      const srcPath = path.join(templatePath, file);
      const destPath = path.join(targetPath, file);
      const stat = await fs.stat(srcPath);

      if (stat.isDirectory()) {
        await fs.ensureDir(destPath);
        await this.copyTemplate(srcPath, destPath);
      } else {
        await this.processFile(srcPath, destPath);
      }
    }
  }

  async processFile(srcPath, destPath) {
    const content = await fs.readFile(srcPath, 'utf-8');
    
    await fs.writeFile(destPath, content);
    console.log(chalk.cyan(`Created: ${path.relative(process.cwd(), destPath)}`));
  }
}

const generator = new FileGenerator();

module.exports = {
  generate: (outputPath, template) => generator.generate({ outputPath, template }),
};