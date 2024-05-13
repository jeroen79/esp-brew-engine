/* eslint-disable import/prefer-default-export */
// automatic detection of browser language
export function getBrowserLocale(options = {}) {
  const defaultOptions = { countryCodeOnly: false };
  const opt = { ...defaultOptions, ...options };

  const navigatorLocale = navigator.languages !== undefined ? navigator.languages[0] : navigator.language;

  console.log('locale', navigatorLocale);

  if (!navigatorLocale) {
    return undefined;
  }

  const trimmedLocale = opt.countryCodeOnly ? navigatorLocale.trim().split(/-|_/)[0] : navigatorLocale.trim();

  return trimmedLocale;
}
