import React from 'react';
import InitColorSchemeScript from '@mui/material/InitColorSchemeScript'
import { ThemeProvider } from '@mui/material/styles';
import theme from '@site/src/theme/muiTheme';

export default function Root({ children }) {
  return (
    <>
      <InitColorSchemeScript />
      <ThemeProvider theme={theme}>{children}</ThemeProvider>
    </>
  );
}
