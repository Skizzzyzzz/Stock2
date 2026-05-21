const API_URL = 'http://localhost:8081/api';

function formatNumber(num) {
    if (num >= 1e12) return (num / 1e12).toFixed(2) + 'T';
    if (num >= 1e9) return (num / 1e9).toFixed(2) + 'B';
    if (num >= 1e6) return (num / 1e6).toFixed(2) + 'M';
    if (num >= 1e3) return (num / 1e3).toFixed(2) + 'K';
    return num.toFixed(2);
}

function formatMarketCap(marketCap) {
    // Handle null, undefined, 0, or unreasonably small numbers
    // Numbers like 3e-314 are basically 0 due to floating point errors
    if (!marketCap || marketCap < 1e6) {  // Less than 1 million is invalid for market cap
        return 'N/A';
    }
    return '$' + formatNumber(marketCap);
}

function getSignalClass(signal) {
    if (signal.includes('BUY')) return 'buy';
    if (signal.includes('SELL')) return 'sell';
    return 'hold';
}

function getSignalEmoji(signal) {
    if (signal === 'STRONG BUY') return '🚀';
    if (signal === 'BUY') return '📈';
    if (signal === 'STRONG SELL') return '⚠️';
    if (signal === 'SELL') return '📉';
    return '⏸️';
}

async function updateStockDashboard() {
    const loading = document.getElementById('loading');
    const container = document.getElementById('stock-list');
    
    try {
        loading.style.display = 'block';
        container.innerHTML = '';
        
        // Calling the C++ route we defined in main.cpp
        const response = await fetch(`${API_URL}/stocks`);
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const data = await response.json();
        loading.style.display = 'none';

        if (data.length === 0) {
            container.innerHTML = '<p class="no-data">No stock data available</p>';
            return;
        }

        // Update statistics
        updateStatistics(data);

        // Display stock cards
        data.forEach(stock => {
            const changeClass = stock.change >= 0 ? 'positive' : 'negative';
            const changeSign = stock.change >= 0 ? '+' : '';
            const signalClass = getSignalClass(stock.signal);
            const signalEmoji = getSignalEmoji(stock.signal);
            
            const card = document.createElement('div');
            card.className = 'stock-card';
            card.innerHTML = `
                <div class="stock-header">
                    <div>
                        <h3>${stock.symbol}</h3>
                        <p class="stock-name">${stock.name || stock.symbol}</p>
                    </div>
                    <div class="stock-price">
                        <h2>$${stock.price.toFixed(2)}</h2>
                        <span class="change ${changeClass}">
                            ${changeSign}${stock.change.toFixed(2)}%
                        </span>
                    </div>
                </div>
                <div class="stock-details">
                    <div class="detail-item">
                        <span class="label">Volume:</span>
                        <span class="value">${formatNumber(stock.volume)}</span>
                    </div>
                    <div class="detail-item">
                        <span class="label">Market Cap:</span>
                        <span class="value">${formatMarketCap(stock.marketCap)}</span>
                    </div>
                </div>
                <div class="stock-signal ${signalClass}">
                    ${signalEmoji} <strong>${stock.signal}</strong>
                </div>
            `;
            container.appendChild(card);
        });
        
        // Update last update time
        const now = new Date();
        document.getElementById('last-update').textContent = now.toLocaleTimeString();
        
    } catch (error) {
        loading.style.display = 'none';
        container.innerHTML = `<p class="error">Failed to fetch stocks: ${error.message}</p>`;
        console.error("Failed to fetch stocks:", error);
    }
}

function updateStatistics(stocks) {
    document.getElementById('total-stocks').textContent = stocks.length;
    
    const buySignals = stocks.filter(s => s.signal.includes('BUY')).length;
    const sellSignals = stocks.filter(s => s.signal.includes('SELL')).length;
    
    document.getElementById('buy-signals').textContent = buySignals;
    document.getElementById('sell-signals').textContent = sellSignals;
}

async function searchStock() {
    const searchInput = document.getElementById('search-input');
    const symbol = searchInput.value.trim().toUpperCase();
    const resultContainer = document.getElementById('search-result');
    
    if (!symbol) {
        alert('Please enter a stock symbol');
        return;
    }
    
    try {
        resultContainer.innerHTML = '<p>Searching...</p>';
        
        const response = await fetch(`${API_URL}/stock/${symbol}`);
        
        if (!response.ok) {
            throw new Error('Stock not found');
        }
        
        const stock = await response.json();
        
        const changeClass = stock.change >= 0 ? 'positive' : 'negative';
        const changeSign = stock.change >= 0 ? '+' : '';
        const signalClass = getSignalClass(stock.signal);
        const signalEmoji = getSignalEmoji(stock.signal);
        
        resultContainer.innerHTML = `
            <h2>Search Result</h2>
            <div class="stock-card featured">
                <div class="stock-header">
                    <div>
                        <h3>${stock.symbol}</h3>
                        <p class="stock-name">${stock.name || stock.symbol}</p>
                    </div>
                    <div class="stock-price">
                        <h2>$${stock.price.toFixed(2)}</h2>
                        <span class="change ${changeClass}">
                            ${changeSign}${stock.change.toFixed(2)}%
                        </span>
                    </div>
                </div>
                <div class="stock-details">
                    <div class="detail-item">
                        <span class="label">Volume:</span>
                        <span class="value">${formatNumber(stock.volume)}</span>
                    </div>
                    <div class="detail-item">
                        <span class="label">Market Cap:</span>
                        <span class="value">${formatMarketCap(stock.marketCap)}</span>
                    </div>
                </div>
                <div class="stock-signal ${signalClass}">
                    ${signalEmoji} <strong>${stock.signal}</strong>
                </div>
            </div>
        `;
        
    } catch (error) {
        resultContainer.innerHTML = `<p class="error">Error: ${error.message}</p>`;
    }
}

// Allow Enter key to trigger search
document.addEventListener('DOMContentLoaded', () => {
    const searchInput = document.getElementById('search-input');
    searchInput.addEventListener('keypress', (e) => {
        if (e.key === 'Enter') {
            searchStock();
        }
    });
});

// Update data every 60 seconds
setInterval(updateStockDashboard, 60000);
updateStockDashboard();