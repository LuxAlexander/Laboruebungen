/*
 * Bootstrap 5.3 color modes: light, dark or auto (follows the operating system).
 * Adapted from the snippet in the Bootstrap docs. The choice is kept in localStorage and
 * applied to <html data-bs-theme="..."> before the page renders to avoid a flash.
 */
(() => {
  'use strict'

  const STORAGE_KEY = 'theme'

  const getStoredTheme = () => localStorage.getItem(STORAGE_KEY)
  const setStoredTheme = theme => localStorage.setItem(STORAGE_KEY, theme)

  const systemTheme = () =>
    window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light'

  const getPreferredTheme = () => getStoredTheme() || 'auto'

  const applyTheme = theme => {
    document.documentElement.setAttribute('data-bs-theme', theme === 'auto' ? systemTheme() : theme)
  }

  const showActiveTheme = theme => {
    const activeIcon = document.querySelector('[data-bs-theme-icon]')
    const activeButton = document.querySelector(`[data-bs-theme-value="${theme}"]`)
    if (!activeButton) {
      return
    }
    document.querySelectorAll('[data-bs-theme-value]').forEach(button => {
      button.classList.remove('active')
      button.setAttribute('aria-pressed', 'false')
    })
    activeButton.classList.add('active')
    activeButton.setAttribute('aria-pressed', 'true')
    if (activeIcon) {
      activeIcon.className = activeButton.querySelector('i').className + ' me-1'
    }
  }

  applyTheme(getPreferredTheme())

  window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', () => {
    if (getPreferredTheme() === 'auto') {
      applyTheme('auto')
    }
  })

  window.addEventListener('DOMContentLoaded', () => {
    showActiveTheme(getPreferredTheme())
    document.querySelectorAll('[data-bs-theme-value]').forEach(button => {
      button.addEventListener('click', () => {
        const theme = button.getAttribute('data-bs-theme-value')
        setStoredTheme(theme)
        applyTheme(theme)
        showActiveTheme(theme)
      })
    })
  })
})()
