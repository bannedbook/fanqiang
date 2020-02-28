/* String format */
String.prototype.format = function() {
    var newStr = this, i = 0;
    while (/%s/.test(newStr)) {
        newStr = newStr.replace("%s", arguments[i++])
    }
    return newStr;
}

/* JavaScript for UI */
function title(title) {
    $('#title').text(title);
}

function tip(message, type, allowOff) {
    if( allowOff === undefined ) {
        allowOff = true;
    }
    if( type === undefined ) {
        type = 'info';
    }

    $('#tip').removeClass('alert-info');
    $('#tip').removeClass('alert-warning');
    $('#tip').removeClass('alert-success');
    $('#tip').removeClass('alert-error');
    $('#tip').removeClass('hide');

    $('#tip').addClass('alert-' + type);

    $('#tip-message').html(message);

    if( allowOff === true ) {
        $('#tip-close').css('display', '');
    } else {
        $('#tip-close').css('display', 'none');
    }
}
function tipClose() {
    $('#tip').addClass('hide');
}
function tipHasClose() {
    return $('#tip').hasClass('hide');
}

$('#tip-close').click(function() {
    tipClose();
});