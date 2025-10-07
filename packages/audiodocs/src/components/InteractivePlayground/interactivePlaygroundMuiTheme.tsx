import React from "react";
import { createTheme, ThemeProvider } from "@mui/material/styles";
import { useColorMode } from "@docusaurus/theme-common";

const lightTheme = createTheme({
  palette: {
    mode: "light",
    primary: {
      main: "#38acdd", // var(--swm-blue-light-100)
      light: "#5bb9e0", // var(--swm-blue-light-80)
      dark: "#126893", // var(--swm-blue-dark-120)
      contrastText: "#fcfcff", // var(--swm-white)
    },
    background: {
      paper: "#fcfcff", //  var(--swm-white)
    },
    text: {
      primary: "#000000",
    },
  },
});

const darkTheme = createTheme({
  palette: {
    mode: "dark",
    primary: {
      main: "#38acdd",
      light: "#5bb9e0",
      dark: "#126893",
      contrastText: "#fcfcff",
    },
    background: {
      paper: "#232736", //  var(--swm-navy)
    },
    text: {
      primary: "#eef0ff", //  var(--swm-navy-light-10)
    },
  },
});

export default function LocalMuiTheme({ children }) {
  const { colorMode } = useColorMode();
  const theme = colorMode === "dark" ? darkTheme : lightTheme;

  return <ThemeProvider theme={theme}>{children}</ThemeProvider>;
}
