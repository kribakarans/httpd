/* Scripts for entry.html */

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

async function kpass_add_list(entries) {
    console.log(`Input: ${JSON.stringify(entries)}`);

    // Get the container where cards will be added
    const container = document.getElementById('card-container');
    container.innerHTML = ''; // Clear any existing content

    // Loop through each entry and create a card
    entries.forEach(entry => {
        // Create card elements
        const card = document.createElement('div');
        card.className = 'card';
        card.id = `entry-${entry.id}`;

        const url = document.createElement('a');
        url.textContent = `${entry.url}`;
        url.className = 'url';
        //url.href = entry.url;
        //url.target = '_blank'; // Open link in a new tab
        //url.rel = 'noopener noreferrer';

        const user = document.createElement('p');
        user.className = 'username';
        user.textContent = `${entry.user}`;

        const passwordContainer = document.createElement('div');
        passwordContainer.className = 'password-container';

        const password = document.createElement('p');
        password.className = 'password';
        password.textContent = `********`;
        password.dataset.hidden = 'true'; // Default to hidden

        // Create the view button
        const viewButton = document.createElement('button');
        viewButton.className = 'view-button';
        viewButton.setAttribute('aria-label', 'Toggle password visibility');
        viewButton.innerHTML = `
            <svg id="visibility-icon" xmlns="http://www.w3.org/2000/svg" height="24px" viewBox="0 -960 960 960" width="24px" fill="#5f6368">
                <path d="M480-320q75 0 127.5-52.5T660-500q0-75-52.5-127.5T480-680q-75 0-127.5 52.5T300-500q0 75 52.5 127.5T480-320Zm0-72q-45 0-76.5-31.5T372-500q0-45 31.5-76.5T480-608q45 0 76.5 31.5T588-500q0 45-31.5 76.5T480-392Zm0 192q-146 0-266-81.5T40-500q54-137 174-218.5T480-800q146 0 266 81.5T920-500q-54 137-174 218.5T480-200Zm0-300Zm0 220q113 0 207.5-59.5T832-500q-50-101-144.5-160.5T480-720q-113 0-207.5 59.5T128-500q50 101 144.5 160.5T480-280Z"/>
            </svg>`;
        viewButton.onclick = () => toggle_password(entry.id, password, viewButton);

        const notes = document.createElement('p');
        notes.textContent = `${entry.notes}`;
        notes.className = 'misc';

        const modtime = document.createElement('p');
        modtime.textContent = `Last Modified: ${entry.modtime}`;
        modtime.className = 'misc';

        // Append elements to the card
        card.appendChild(url);
        //card.appendChild(title);
        card.appendChild(user);
        card.appendChild(password);
        card.appendChild(viewButton);
        card.appendChild(passwordContainer);
        card.appendChild(notes);
        card.appendChild(modtime);

        // Append password elements
        passwordContainer.appendChild(password);
        passwordContainer.appendChild(viewButton);

        // Append card to the container
        container.appendChild(card);
    });
}

async function toggle_password(id, passwordElement, button) {
    const isHidden = passwordElement.dataset.hidden === 'true';

    if (isHidden) {
        await get_entry_password(id, passwordElement, button);
        button.innerHTML = `
            <svg id="visibility-icon" xmlns="http://www.w3.org/2000/svg" height="24px" viewBox="0 -960 960 960" width="24px" fill="#5f6368">
                <path d="m644-428-58-58q9-47-27-88t-93-32l-58-58q17-8 34.5-12t37.5-4q75 0 127.5 52.5T660-500q0 20-4 37.5T644-428Zm128 126-58-56q38-29 67.5-63.5T832-500q-50-101-143.5-160.5T480-720q-29 0-57 4t-55 12l-62-62q41-17 84-25.5t90-8.5q151 0 269 83.5T920-500q-23 59-60.5 109.5T772-302Zm20 246L624-222q-35 11-70.5 16.5T480-200q-151 0-269-83.5T40-500q21-53 53-98.5t73-81.5L56-792l56-56 736 736-56 56ZM222-624q-29 26-53 57t-41 67q50 101 143.5 160.5T480-280q20 0 39-2.5t39-5.5l-36-38q-11 3-21 4.5t-21 1.5q-75 0-127.5-52.5T300-500q0-11 1.5-21t4.5-21l-84-82Zm319 93Zm-151 75Z"/>
            </svg>
            `;

    } else {
        passwordElement.textContent = `********`; // Hide password
        button.innerHTML = `
            <svg id="visibility-icon" xmlns="http://www.w3.org/2000/svg" height="24px" viewBox="0 -960 960 960" width="24px" fill="#5f6368">
                <path d="M480-320q75 0 127.5-52.5T660-500q0-75-52.5-127.5T480-680q-75 0-127.5 52.5T300-500q0 75 52.5 127.5T480-320Zm0-72q-45 0-76.5-31.5T372-500q0-45 31.5-76.5T480-608q45 0 76.5 31.5T588-500q0 45-31.5 76.5T480-392Zm0 192q-146 0-266-81.5T40-500q54-137 174-218.5T480-800q146 0 266 81.5T920-500q-54 137-174 218.5T480-200Zm0-300Zm0 220q113 0 207.5-59.5T832-500q-50-101-144.5-160.5T480-720q-113 0-207.5 59.5T128-500q50 101 144.5 160.5T480-280Z"/>
                </svg>
            `;
        passwordElement.dataset.hidden = 'true'; // Hide password
    }
}

async function get_entry_password(id, passwordElement, button) {
    fetch(`/entry/${id}`)
        .then(response => {
            console.log(`Get entry: ${id}`);
            if (!response.ok) {
                throw new Error(`Error fetching password: ${response.status} ${response.statusText}`);
            }
            return response.json(); // Parse response as JSON
        })
        .then(data => {
            console.log('Server response:', data);

            if (data && typeof data === 'object' && data.id === id) {
                console.log(`Matched entry ID: ${data.id}`);
                passwordElement.textContent = `${data.password}`;
                passwordElement.dataset.hidden = 'false'; // Show password
            } else {
                console.log(`Unexpected data received: ${JSON.stringify(data)}`);
                passwordElement.textContent = 'Password not available';
                kpass_error("Password not available");
            }
        })
        .catch(error => {
            kpass_error(`${error}`);
        });
}

function getParameterByName(name, url) {
    if (!url) url = window.location.href;
    name = name.replace(/[\[\]]/g, '\\$&');
    const regex = new RegExp('[?&]' + name + '(=([^&#]*)|&|#|$)'),
        results = regex.exec(url);
    if (!results) return null;
    if (!results[2]) return '';
    return decodeURIComponent(results[2].replace(/\+/g, ' '));
}

/* EOF */
