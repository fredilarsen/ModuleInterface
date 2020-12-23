// This function sends modified values back to the server
$(function() {
  //var message_status = $("#statusText");
  $("td[contenteditable=true]").blur(function() {
    var field_id = $(this).attr("id");
    var value = $(this).text();
    console.log("Storing to db " + field_id + " value " + value);
    $(this).removeClass("userEditing");
    $.post('set_setting.php', field_id + "=" + value, function(data) {

      //            if(data != '')
      //      {
      //        message_status.show();
      //        message_status.text("Setting " + field_id + ", " + data);
      //hide the message
      //        setTimeout(function(){message_status.hide()},15000);
      //      }
    });
  });
});

// This function marks editable fields when focused
$(function() {
  $("td[contenteditable=true]").focus(function() {
    $(this).addClass("userEditing");
  });
});

// This function fills values from the database into the fields
$(document).ready(function() {
  updateTablesWithMeasurements();
  updateTablesWithSettings();
  setInterval(updateTablesWithMeasurements, 5000);
  setInterval(updateTablesWithSettings, 5000);
});

function pad(number) {
  return ((number < 10) ? '0' : '') + number;
};

Date.prototype.toLocaleISOString = function() {
  return this.getFullYear() +
    '-' + pad(this.getMonth() + 1) +
    '-' + pad(this.getDate()) +
    ' ' + pad(this.getHours()) +
    ':' + pad(this.getMinutes()) +
    ':' + pad(this.getSeconds());
};

var startedUpdating = 0;
var startedUpdatingSettings = 0;

function updateTablesWithMeasurements() {
  // Avoid updating if last attempt is not complete
  var n = new Date().getMilliseconds();
  if (startedUpdating > 0) {
    if (n - startedUpdating < 60000) {
      console.log("Outputs already updating!");
      return;
    } else console.log("Outputs already updating, but has timed out!");
  }
  startedUpdating = n;

  var table_meas = $("#hhMeasurements");
  var table_outputs = $("#hhOutputs");
  var table_inputs = $("#hhInputs");
  $.getJSON("get_currentvalues.php", function(data) {
      $.each(data, function(i, item) {
        if (table_meas.length) {
          var cell = table_meas.find('[id="' + i + '"]');
          if (cell.length > 0) cell.text(item);
        }
        if (table_outputs.length) {
          var cell = table_outputs.find('[id="' + i + '"]');
          if (cell.length > 0) {
            if (i.indexOf("LastLife") != -1) {
              var d = new Date(1000 * item);
              cell.text(d.toLocaleISOString());
            } else cell.text(item);
          }
        }
        if (table_inputs.length) {
          var cell = table_inputs.find('[id="' + i + '"]');
          if (cell.length > 0) cell.text(item);
        }
      });
    })
    .always(function() {
      startedUpdating = 0;
    });
}

function updateTablesWithSettings() {
  var table_settings = $("#hhSettings");
  if (table_settings.length) {
    // Avoid updating if last attempt is not complete
    var n = new Date().getMilliseconds();
    if (startedUpdatingSettings > 0) {
      if (n - startedUpdatingSettings < 60000) {
        console.log("Settings already updating!");
        return;
      } else console.log("Settings already updating, but has timed out!");
    }
    startedUpdatingSettings = n;

    $.getJSON("get_settings.php", function(data) {
        $.each(data, function(i, item) {
          var cell = table_settings.find('[id="' + i + '"]');
          if (cell.length > 0 && !cell.hasClass("userEditing")) { // Do not change if user is editing
            cell.text(item);
            postValueChangeAction(i, cell);
          }
        });
      })
      .always(function() {
        startedUpdatingSettings = 0;
      });
  }
}

// Load error handlers (for import statements)
function handleLoad(e) {
  console.log('Loaded import: ' + e.target.href);
}

function handleError(e) {
  console.log('Error loading import: ' + e.target.href);
}

function getMinutesOfDayAsInfo(minutesOfDay) {
  if (minutesOfDay == null || minutesOfDay == "") return "";
  var hours = Math.floor(minutesOfDay / 60),
    minutes = Math.floor(minutesOfDay - hours * 60);
  return (hours <= 9 ? "0" + hours : "" + hours) + ":" +
    (minutes <= 9 ? "0" + minutes : "" + minutes);
}

function getUTCAsTimeInfo(UTC) {
  if (new Date(1000 * UTC) < new Date()) return "Off";
  return "+" + Math.round((new Date(1000 * UTC).getTime() - new Date().getTime()) / 60000) + " min"; //toISOString().slice(0,-5).replace('T',' ');
}

function getQueryVariable(variable) {
  var query = window.location.search.substring(1);
  var vars = query.split("&");
  for (var i = 0; i < vars.length; i++) {
    var pair = vars[i].split("=");
    if (pair[0] == variable) {
      return pair[1];
    }
  }
  return (false);
}

function highlightPage(activeId) {
  document.getElementById(activeId).classList.add("active");
};
