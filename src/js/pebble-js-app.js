/* Copyright 2014 Andreas Muttscheller

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

var initialized = false;

// code.stephenmorley.org
function Queue() {
  var a = [], b = 0;
  this.getLength = function() {return a.length - b};
  this.isEmpty = function() {return 0 == a.length};
  this.enqueue = function(b) {a.push(b)};
  this.dequeue = function() {
    if (0 != a.length) {
      var c = a[b];
      2 * ++b >= a.length&&(a = a.slice(b), b = 0);
      return c
    }
  };
  this.peek = function() { return 0 < a.length ? a[b] : void 0 }
};

var messageQueue = new Queue();

function ackHandler(e) {
  sendNextMessage();
}

function nackHandler(e) {
  console.log("Unable to deliver message with transactionId=" +
              e.data.transactionId);
}

function sendNextMessage() {
  if (messageQueue.isEmpty()) return;

  var msg = messageQueue.dequeue();

  var t = Pebble.sendAppMessage(msg, ackHandler, nackHandler);
}

function fetchCurrencyQuotes() {
  var req = new XMLHttpRequest();
  
  req.onerror = function() {
    Pebble.sendAppMessage({"errorSending": 1});
  }
  
  req.open('GET',
           'https://www.ecb.europa.eu/stats/eurofxref/eurofxref-daily.xml',
           true);
           
  req.onload = function(e) {
    if (req.readyState == 4 && req.status == 200) {
      if (req.status == 200) {
        var response = req.responseXML;
        var cubes = response.getElementsByTagName(
                                 "Cube")[0].getElementsByTagName("Cube");

        var length = cubes.length;
        // Send length
        messageQueue.enqueue({"currencyLength": length});

        for (var i = 1; i < cubes.length; ++i) {
          messageQueue.enqueue({
            "currencyIndex": i,
            "currency": cubes[i].getAttribute("currency"),
            "currencyRate": cubes[i].getAttribute("rate")
          });
        }

        // Add EUR as well
        messageQueue.enqueue(
            {"currencyIndex": i, "currency": "EUR", "currencyRate": "1.0"});

        // Finished message
        messageQueue.enqueue({"currencyDone": 1});
        sendNextMessage();

      } else {
        console.log("Error");
        Pebble.sendAppMessage({"errorSending": 1});
      }
    } else {
      Pebble.sendAppMessage({"errorSending": 1});
    }
  }
  req.send(null);
}

Pebble.addEventListener("ready", function() {
  console.log("ready called!");
  initialized = true;
});

Pebble.addEventListener("appmessage", function(e) {
  console.log("Received message: " + e.payload);

  if (e.payload.updateCurrency) {
    fetchCurrencyQuotes();
  }
});
