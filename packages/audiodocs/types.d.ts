

/// <reference types="@docusaurus/module-type-aliases" />
/// <reference types="@docusaurus/theme-classic" />

module "*.module.css" {
  const classes: { [key: string]: string };
  export default classes;
}

module "*.svg" {
  import React from "react";
  const ReactComponent: React.FC<React.SVGProps<SVGSVGElement>>;
  export default ReactComponent;
}

// t-rex-ui's own typings don't resolve here; declare what the TypeScript files use.
declare module '@swmansion/t-rex-ui' {
  import type { ComponentType, ReactNode } from 'react';

  export interface BannerZone {
    zoneId: string;
    contentId: string;
    fallbackBgColor: string;
  }

  export const HireUsSection: ComponentType<{ content: ReactNode; href: string }>;
}
declare module '@swmansion/t-rex-ui/topbar-banner';