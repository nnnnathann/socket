/* global MutationObserver */
import { fetch, Headers, Request, Response } from '../fetch.js'
import { URL, URLPattern, URLSearchParams } from '../url.js'
import ipc from '../ipc.js'

let applied = false

export function init () {
  if (applied || !globalThis.window) return

  Object.assign(globalThis, {
    // url
    URL,
    URLPattern,
    URLSearchParams,

    // fetch
    fetch,
    Headers,
    Request,
    Response
  })

  applied = true
  // create <title> tag in document if it doesn't exist
  globalThis.document.title ||= ''
  // initial value
  globalThis.addEventListener('DOMContentLoaded', async () => {
    const title = globalThis.document.title
    if (title.length !== 0) {
      const index = globalThis.__args.index
      const o = new URLSearchParams({ value: title, index }).toString()
      ipc.postMessage(`ipc://window.setTitle?${o}`)
    }
  })

  //
  // globalThis.document is uncofigurable property so we need to use MutationObserver here
  //
  const observer = new MutationObserver((mutationList) => {
    for (const mutation of mutationList) {
      if (mutation.type === 'childList') {
        const index = globalThis.__args.index
        const title = mutation.addedNodes[0].textContent
        const o = new URLSearchParams({ value: title, index }).toString()
        ipc.postMessage(`ipc://window.setTitle?${o}`)
      }
    }
  })

  const titleElement = document.querySelector('head > title')
  if (titleElement) {
    observer.observe(titleElement, { childList: true })
  }
}

export default {
  init
}
