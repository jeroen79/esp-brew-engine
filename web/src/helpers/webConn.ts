// Wrapper function for webrequests so we can change the used class or total backend logic
import { IApiResult } from '@/interfaces/IApiResult';

export default class WebConn {
  public rootUrl: string | null = null;

  constructor(rootUrl: string) {
    this.rootUrl = rootUrl;
  }

  doPostRequest(data: any): Promise<IApiResult> {
    return new Promise((resolve, reject) => {
      const url = `${this.rootUrl}api`;

      const response = fetch(url, {
        method: 'POST', // *GET, POST, PUT, DELETE, etc.
        mode: 'cors', // no-cors, *cors, same-origin //no-cors doesn't give any data, only gives error about json parse (bug?)
        cache: 'no-cache', // *default, no-cache, reload, force-cache, only-if-cached
        credentials: 'omit', // include, *same-origin, omit
        headers: {
          'Content-Type': 'application/json',
        },
        // redirect: "follow", // manual, *follow, error
        // referrerPolicy: "no-referrer", // no-referrer, *no-referrer-when-downgrade, origin, origin-when-cross-origin, same-origin, strict-origin, strict-origin-when-cross-origin, unsafe-url
        body: JSON.stringify(data), // body data type must match "Content-Type" header
      }).then((result) => {
        const apiResult = result.json();
        resolve(apiResult);
      }).catch((error) => {
        console.error(error);
        const apiResult: IApiResult = {
          success: false,
          data: null,
          message: error,
        };
        reject(apiResult);
      });
    });
  }
}
