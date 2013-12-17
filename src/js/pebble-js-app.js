var maxAppMessageTries = 3;
var appMessageRetryTimeout = 3000;
var appMessageTimeout = 100;
var appMessageQueue = [];

function sendAppMessage() {
	if (appMessageQueue.length > 0) {
		currentAppMessage = appMessageQueue[0];
		currentAppMessage.numTries = currentAppMessage.numTries || 0;
		currentAppMessage.transactionId = currentAppMessage.transactionId || -1;
		if (currentAppMessage.numTries < maxAppMessageTries) {
			console.log('Sending AppMessage to Pebble: ' + JSON.stringify(currentAppMessage.message));
			Pebble.sendAppMessage(
				currentAppMessage.message,
				function(e) {
					appMessageQueue.shift();
					setTimeout(function() {
						sendAppMessage();
					}, appMessageTimeout);
				}, function(e) {
					console.log('Failed sending AppMessage for transactionId:' + e.data.transactionId + '. Error: ' + e.data.error.message);
					appMessageQueue[0].transactionId = e.data.transactionId;
					appMessageQueue[0].numTries++;
					setTimeout(function() {
						sendAppMessage();
					}, appMessageRetryTimeout);
				}
			);
		} else {
			console.log('Failed sending AppMessage for transactionId:' + currentAppMessage.transactionId + '. Bailing. ' + JSON.stringify(currentAppMessage.message));
		}
	}
}

function fetch() {
	var xhr = new XMLHttpRequest();
	xhr.open('GET', 'http://api.breakingnews.com/api/v1/item/', true);
	xhr.onload = function(e) {
		if (xhr.readyState == 4) {
			if (xhr.status == 200) {
				if (xhr.responseText) {
					res = JSON.parse(xhr.responseText);
					res.objects.forEach(function (element, index, array) {
						content = element.content.substring(0,159);
						appMessageQueue.push({'message': {'index': index, 'content': content}});
					});
				} else {
					console.log('Invalid response received! ' + JSON.stringify(xhr));
					appMessageQueue.push({'message': {'error': 'Invalid response!'}});
				}
			} else {
				console.log('Request returned error code ' + xhr.status.toString());
				appMessageQueue.push({'message': {'error': 'HTTP/1.1 ' + xhr.statusText}});
			}
		}
		sendAppMessage();
	}
	xhr.onerror = function() {
		console.log('HTTP request return error');
		appMessageQueue.push({'message': {'error': 'Failed to connect!'}});
		sendAppMessage();
	};
	xhr.send(null);
}

Pebble.addEventListener('ready', function(e) {
	fetch();
});

Pebble.addEventListener('appmessage', function(e) {
	appMessageQueue = [];
	fetch();
});

