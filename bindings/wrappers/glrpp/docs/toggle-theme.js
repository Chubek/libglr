(function() {
    const THEME_KEY = 'doxygen-theme';
    const darkCSS = 'doxygen-dark.css';
    const lightCSS = ''; // Doxygen default
    
    function setTheme(isDark) {
        const link = document.getElementById('theme-stylesheet');
        if (isDark) {
            if (!link) {
                const newLink = document.createElement('link');
                newLink.id = 'theme-stylesheet';
                newLink.rel = 'stylesheet';
                newLink.href = darkCSS;
                document.head.appendChild(newLink);
            }
        } else {
            if (link) link.remove();
        }
        localStorage.setItem(THEME_KEY, isDark ? 'dark' : 'light');
    }
    
    function createToggle() {
        const toggle = document.createElement('button');
        toggle.id = 'theme-toggle';
        toggle.innerHTML = '🌙';
        toggle.style.cssText = `
            position: fixed;
            top: 10px;
            right: 10px;
            z-index: 1000;
            padding: 8px 12px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 1.2em;
            background: var(--bg-tertiary);
            color: var(--text-primary);
        `;
        
        toggle.addEventListener('click', () => {
            const isDark = localStorage.getItem(THEME_KEY) !== 'dark';
            setTheme(isDark);
            toggle.innerHTML = isDark ? '☀️' : '🌙';
        });
        
        document.body.appendChild(toggle);
    }
    
    // Initialize
    const savedTheme = localStorage.getItem(THEME_KEY);
    const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
    const isDark = savedTheme === 'dark' || (!savedTheme && prefersDark);
    
    setTheme(isDark);
    window.addEventListener('DOMContentLoaded', createToggle);
})();
