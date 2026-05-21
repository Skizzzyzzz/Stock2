const AUTH_API_URL = 'http://localhost:8081/api';

function checkAuth() {
    const token    = localStorage.getItem('token');
    const username = localStorage.getItem('username');

    if (token && username && window.location.pathname.includes('index.html')) return true;
    if (!token && window.location.pathname.includes('index.html')) {
        window.location.href = 'login.html';
        return false;
    }
    return !!token;
}

if (document.getElementById('login-form')) {
    document.getElementById('login-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        const username = document.getElementById('username').value;
        const password = document.getElementById('password').value;
        const errorDiv = document.getElementById('error-message');

        try {
            const response = await fetch(`${AUTH_API_URL}/auth/login`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ username, password })
            });
            const data = await response.json();
            if (data.success) {
                localStorage.setItem('token', data.token);
                localStorage.setItem('username', data.username);
                window.location.href = 'index.html';
            } else {
                errorDiv.textContent  = data.message || 'Login failed';
                errorDiv.style.display = 'block';
            }
        } catch {
            errorDiv.textContent   = 'Connection error. Please try again.';
            errorDiv.style.display = 'block';
        }
    });
}

if (document.getElementById('signup-form')) {
    document.getElementById('signup-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        const username        = document.getElementById('username').value;
        const email           = document.getElementById('email').value;
        const password        = document.getElementById('password').value;
        const confirmPassword = document.getElementById('confirm-password').value;
        const errorDiv        = document.getElementById('error-message');
        const successDiv      = document.getElementById('success-message');

        errorDiv.style.display   = 'none';
        successDiv.style.display = 'none';

        if (password !== confirmPassword) {
            errorDiv.textContent   = 'Passwords do not match';
            errorDiv.style.display = 'block';
            return;
        }
        if (password.length < 6) {
            errorDiv.textContent   = 'Password must be at least 6 characters';
            errorDiv.style.display = 'block';
            return;
        }

        try {
            const response = await fetch(`${AUTH_API_URL}/auth/register`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ username, email, password })
            });
            const data = await response.json();
            if (data.success) {
                localStorage.setItem('token', data.token);
                localStorage.setItem('username', data.username);
                successDiv.textContent   = 'Account created successfully! Redirecting...';
                successDiv.style.display = 'block';
                setTimeout(() => { window.location.href = 'index.html'; }, 1500);
            } else {
                errorDiv.textContent   = data.message || 'Registration failed';
                errorDiv.style.display = 'block';
            }
        } catch {
            errorDiv.textContent   = 'Connection error. Please try again.';
            errorDiv.style.display = 'block';
        }
    });
}

function logout() {
    localStorage.removeItem('token');
    localStorage.removeItem('username');
    window.location.href = 'login.html';
}

const logoutButton = document.getElementById('logout-btn');
if (logoutButton) logoutButton.addEventListener('click', logout);

window.checkAuth = checkAuth;
window.logout    = logout;
