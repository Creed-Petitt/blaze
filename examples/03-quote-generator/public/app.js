// Fetch and display a random quote
async function fetchQuote() {
    try {
        const response = await fetch('/api/quote');
        const data = await response.json();

        document.getElementById('quote-text').textContent = data.quote;
        document.getElementById('total-quotes').textContent = data.total;
    } catch (error) {
        document.getElementById('quote-text').textContent = 'Error loading quote!';
        console.error('Error:', error);
    }
}

// Add a new quote
async function addQuote() {
    const input = document.getElementById('new-quote-input');
    const quote = input.value.trim();
    const messageDiv = document.getElementById('message');

    if (!quote) {
        showMessage('Please enter a quote', 'error');
        return;
    }

    try {
        const response = await fetch('/api/quotes', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ quote })
        });

        const data = await response.json();

        if (response.ok) {
            showMessage('Quote added successfully! 🎉', 'success');
            input.value = '';
            fetchQuote(); // Load the new quote
        } else {
            showMessage(data.error || 'Error adding quote', 'error');
        }
    } catch (error) {
        showMessage('Network error', 'error');
        console.error('Error:', error);
    }
}

// Show all quotes in console (for demo purposes)
async function showAllQuotes() {
    try {
        const response = await fetch('/api/quotes');
        const data = await response.json();

        console.log('=== All Quotes ===');
        data.quotes.forEach((item, index) => {
            console.log(`${index + 1}. ${item.quote}`);
        });
        console.log(`\nTotal: ${data.total} quotes`);

        showMessage(`Check the browser console (F12) to see all ${data.total} quotes!`, 'success');
    } catch (error) {
        showMessage('Error loading quotes', 'error');
        console.error('Error:', error);
    }
}

// Show message to user
function showMessage(text, type) {
    const messageDiv = document.getElementById('message');
    messageDiv.textContent = text;
    messageDiv.className = type;

    setTimeout(() => {
        messageDiv.style.display = 'none';
    }, 3000);
}

// Load a quote when page loads
fetchQuote();
