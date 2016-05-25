Pebble.addEventListener('ready', function() {
	console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('showConfiguration', function() {
	var url = 'http://192.168.178.51:8080/greenclock-face-config/';
	//var url = 'http://ylno.github.io/greenclock-face-config/';

	console.log('Showing configuration page: ' + url);

	Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
	var configData = JSON.parse(decodeURIComponent(e.response));

	console.log('Configuration page returned: ' + JSON.stringify(configData));

	var dict = {
		minuteCircleColor: parseInt(configData.minuteCircleColor, 16),
		hourCircleColor: parseInt(configData.hourCircleColor, 16),
		tickColor: parseInt(configData.tickColor, 16),
		batteryColor: parseInt(configData.batteryColor, 16),
		showBatteryLoad: configData.showBatteryLoad ? 1: 0,
		showDigitalDate: configData.showDigitalDate ? 1: 0,
		showDigitalTime: configData.showDigitalTime ? 1: 0,
		showShadow: configData.showShadow ? 1: 0,
		textColor: parseInt(configData.textColor, 16),
		bgColor: parseInt(configData.bgColor, 16),
		clockfaceColor: parseInt(configData.clockfaceColor, 16),
		clockfaceouterColor: parseInt(configData.clockfaceouterColor, 16),
		minutehandColor: parseInt(configData.minutehandColor, 16),
		hourhandColor: parseInt(configData.hourhandColor, 16)
	};

	if (configData.minuteCircleColor) {
		Pebble.sendAppMessage(dict, function() {
			//console.log('Send successful! ' +  JSON.stringify(dict));
		}, function() {
			//console.log('Send failed!');
		});
	}
});
