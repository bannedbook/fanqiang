/**
 * fork from genpac 2.0.1 https://github.com/JinnLynn/genpac
 * GFWList From: online[https://raw.githubusercontent.com/gfwlist/gfwlist/master/gfwlist.txt]
 */

var black_list = [
    "BLACK_LIST"
];

var white_list = [
    "WHITE_LIST"
];

var proxy = 'PROXY PROXY_LISTEN';
function FindProxyForURL(url, host) {

    for (var i = 0; i < white_list.length; i++) {
        var host_end = white_list[i];
        if (host == host_end || host.endsWith('.' + host_end)) {
            return 'DIRECT';
        }
    }

    for (var i = 0; i < black_list.length; i++) {
        var host_end = black_list[i];
        if (host == host_end || host.endsWith('.' + host_end)) {
            return proxy;
        }
    }
    return 'DIRECT';
}


// REF: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String/endsWith
if (!String.prototype.endsWith) {
    String.prototype.endsWith = function(searchString, position) {
        var subjectString = this.toString();
        if (typeof position !== 'number' || !isFinite(position) || Math.floor(position) !== position || position > subjectString.length) {
            position = subjectString.length;
        }
        position -= searchString.length;
        var lastIndex = subjectString.indexOf(searchString, position);
        return lastIndex !== -1 && lastIndex === position;
    };
}
