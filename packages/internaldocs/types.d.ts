/// <reference types="@docusaurus/module-type-aliases" />
/// <reference types="@docusaurus/theme-classic" />

declare module '*.module.css' {
  const classes: { [key: string]: string };
  export default classes;
}
