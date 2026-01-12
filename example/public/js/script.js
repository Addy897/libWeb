

document.getElementById('postButton').addEventListener('click', function() {
  const key1 = document.getElementById('key1').value;
  const key2 = document.getElementById('key2').value;

  if (!key1 || !key2) {
    alert('Please fill out both input fields.');
    return;
  }

  const apiUrl = '/';

  const data = {
    key1: key1,
    key2: key2
  };

  fetch(apiUrl, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify(data)
  })
    .then(response => response.json())
    .then(result => {
      document.getElementById('post_test').innerHTML = `<pre>${JSON.stringify(result, null, 2)}</pre>`;
    })
    .catch(error => {
      document.getElementById('post_test').innerHTML = `<p>Error: ${error.message}</p>`;
    });
});

