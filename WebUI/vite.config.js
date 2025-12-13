import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// Google Analytics script to inject
const googleAnalyticsScript = `
  <!-- Google tag (gtag.js) -->
  <script async src="https://www.googletagmanager.com/gtag/js?id=G-JY0JWSR0T8"></script>
  <script>
    window.dataLayer = window.dataLayer || [];
    function gtag(){dataLayer.push(arguments);}
    gtag('js', new Date());
    gtag('config', 'G-JY0JWSR0T8');
  </script>
`;

export default defineConfig({
  plugins: [
    react(),
    {
      name: 'inject-google-analytics',
      transformIndexHtml(html) {
        return html.replace('<head>', `<head>${googleAnalyticsScript}`);
      },
    },
  ],
  server: {
    port: 3000,
  },
  build: {
    outDir: 'dist',
  }
})
