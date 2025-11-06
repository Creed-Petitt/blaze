# Quote Generator - Static Files + API Example

Demonstrates how to serve static files (HTML/CSS/JS) alongside a REST API.

## Framework Features Demonstrated

- ✅ `middleware::static_files()` - Serve HTML/CSS/JS
- ✅ `middleware::cors()` - Enable CORS for API calls
- ✅ `app.group("/api")` - Route grouping
- ✅ `res.json()` - JSON responses
- ✅ `res.bad_request()` - Error handling
- ✅ Mixing static content with dynamic API

## Build & Run

```bash
cd examples/03-quote-generator
mkdir build && cd build
cmake ..
make
./quote_generator
```

## Usage

1. Open your browser to `http://localhost:3000`
2. Click "Get New Quote" to see random inspirational quotes
3. Add your own quotes using the form
4. Click "See All" to view all quotes in the console

## Project Structure

```
03-quote-generator/
├── main.cpp           # Backend server
└── public/            # Frontend files
    ├── index.html     # HTML structure
    ├── style.css      # Styles with gradients
    └── app.js         # JavaScript for API calls
```

## API Endpoints

### GET /api/quote
Get a random quote

```bash
curl http://localhost:3000/api/quote
```

**Response:**
```json
{
  "quote": "Code is like humor. When you have to explain it, it's bad. - Cory House",
  "id": 1,
  "total": 8
}
```

### GET /api/quotes
Get all quotes

```bash
curl http://localhost:3000/api/quotes
```

### POST /api/quotes
Add a new quote

```bash
curl -X POST http://localhost:3000/api/quotes \
  -H "Content-Type: application/json" \
  -d '{"quote":"Your inspiring quote here"}'
```

## Key Code Sections

### Static File Middleware
```cpp
app.use(middleware::static_files("./public"));
```
Automatically serves:
- `public/index.html` → `http://localhost:3000/index.html`
- `public/style.css` → `http://localhost:3000/style.css`
- `public/app.js` → `http://localhost:3000/app.js`

### CORS Middleware
```cpp
app.use(middleware::cors());
```
Allows JavaScript on the page to make API calls to the same server.

### API Route Group
```cpp
auto api = app.group("/api");
api.get("/quote", handler);   // Routes to /api/quote
api.post("/quotes", handler);  // Routes to /api/quotes
```

### Serving Both Static + Dynamic
The server handles:
- **Static requests** → Served by `static_files()` middleware
- **API requests** → Handled by route handlers
- All from the same port (3000)

## Customization

### Add More Quotes
Edit `main.cpp` and add to the `quotes` vector:

```cpp
std::vector<std::string> quotes = {
    "Your quote here",
    // ... more quotes
};
```

### Change Styling
Edit `public/style.css` to customize colors, fonts, and layout.

### Add Features
- Database persistence
- User authentication
- Quote categories
- Upvote/downvote system
- Search functionality

## Use Cases

This pattern is useful for:
- Serving SPAs (Single Page Applications) built with React/Vue
- Documentation sites with search APIs
- Admin dashboards
- Any app where you want frontend + backend in one server

Check out the other examples:
- [01-hello-world](../01-hello-world) - Minimal starting point
- [02-todo-api](../02-todo-api) - REST API with CRUD operations
