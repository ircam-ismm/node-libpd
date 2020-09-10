/**
 * read README and inplace replace <!-- api -->(.*)<!-- apiend -->
 * with jsdoc2markdown output
 */
const jsdoc2md = require('jsdoc-to-markdown');
const fs = require('fs');
const path = require('path');

// input and output paths
const inputFile = 'index.js';
const readmeFile = 'README.md';

// get template data
const templateData = jsdoc2md.getTemplateDataSync({ files: inputFile });
const readme = fs.readFileSync(readmeFile).toString();
const tag = 'api';

const hasTagEnd = readme.search(new RegExp(`<!--[ \t]*${tag}stop[ \t]-->`, 'm')) !== -1;

let find;
if (hasTagEnd) {
  find = new RegExp(`<!--[ \t]*${tag}[ \t]-->.*<!--[ \t]*${tag}stop[ \t]-->`, 'ms');
} else {
  find = new RegExp(`<!--[ \t]*${tag}[ \t]-->`, 'm');
}

const templateDataFormatted = `<!-- ${tag} -->

${jsdoc2md.renderSync({
  'heading-depth': 3,
  data: templateData,
  template: '{{>main}}'
})}
<!-- ${tag}stop -->`;

const readmeWithAPI = readme.replace(find, templateDataFormatted);

fs.writeFileSync(readmeFile, readmeWithAPI);
