/* Scripts for index.html */

function kpass_error(error) {
    const errorMessage = `${error.message || error}`;

    console.error(`${errorMessage}`);
    kpass_print(`${errorMessage}`);
}

function kpass_print(message) {
    const statusBar = document.getElementById('statusBar');
    const statusMessage = document.getElementById('statusMessage');
    statusMessage.textContent = message;

    // Optional: Automatically clear the message after a delay
    setTimeout(() => {
        statusMessage.textContent = '';
    }, 10000); // Clear message after 10 seconds
}

function kpass_grep_entry(query) {
    fetch('/grep', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ query: query })
    })
        .then(response => {
            /* Run command 'kpass -j -g '${query}'' */
            if (!response.ok) {
                throw new Error(`Error fetching pattern list: ${response.status} ${response.statusText}`);
            }
            return response.json(); // Parse response as JSON
        })
        .then(data => {
            console.log('Server response:', data);
            const container = document.getElementById('card-container');
            container.innerHTML = ''; // Clear any existing content

            if (! data.length) {
                kpass_print("No match found ...")
                return;
            }

            // Extract field values, remove duplicates, and sort alphabetically
            const domainNames = Array.from(new Set(data.map(jsonobject => jsonobject.name))).sort();

            // Append each domain as a card to the domainList
            domainNames.forEach(domain => {
                if (domain.trim()) { // Ensure it's not an empty line
                    const card = document.createElement('div');
                    card.className = 'card';
                    card.textContent = domain;
                    card.addEventListener('click', () => {
                        kpass_list_entries(domain);
                    });
                    container.appendChild(card);
                }
            });
        })
        .catch(error => {
            kpass_error(`${error}`)
        });
}

function kpass_list_domain() {
    fetch('/list')
        .then(response => {
            /* Run command 'kpass -j -n -L' */
            if (!response.ok) {
                return response.json().then(err => {
                    throw new Error(`${err.error}`);
                    //throw new Error(`Error: ${err.error}, stderr: ${err.stderr}`);
                });
            }

            return response.json(); // Parse response as JSON
        })
        .then(data => {
            console.log(data);
            const container = document.getElementById('card-container');
            container.innerHTML = ''; // Clear any existing content

            if (! data.length) {
                kpass_print("No entries ...")
                return;
            }

            // Extract field values, remove duplicates, and sort alphabetically
            const domainNames = Array.from(new Set(data.map(jsonobject => jsonobject.field))).sort();

            // Append each domain as a card to the domainList
            domainNames.forEach(domain => {
                if (domain.trim()) { // Ensure it's not an empty line
                    const card = document.createElement('div');
                    card.className = 'card';
                    card.textContent = domain;
                    card.addEventListener('click', () => {
                        kpass_list_entries(domain);
                    });
                    container.appendChild(card);
                }
            });
        })
        .catch(error => {
            kpass_error(`${error}`);
        });
}

function kpass_list_entries(domain) {
    fetch('/domain', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },

        body: JSON.stringify({ domain: domain })
    })
        /* Run command 'kpass -j -f 'NAME=${domain}'' */
        .then(response => {
            if (!response.ok) {
                throw new Error(`Error fetching entry: ${response.status} ${response.statusText}`);
            }
            return response.json(); // Parse response as JSON
        })
        .then(data => {
            console.log('Server response:', data);

            // Store the server response in localStorage
            localStorage.setItem('serverResponse', JSON.stringify(data));

            // Redirect to the result page
            window.location.href = `entry.html?domain=${encodeURIComponent(domain)}`;
        })
        .catch(error => {
            kpass_error(`${error}`);
        });
}

/* EOF */
