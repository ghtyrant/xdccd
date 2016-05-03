var app = angular.module("xdccdApp", ['ui.bootstrap']);

app.factory('apiService', function($http) {
  return {
    getBots: function() {
      return $http.get('/status')
        .then(function(result) {
          return result.data;
        });
    },
    disconnect: function(bot_id) {
      $http.get('/disconnect/' + bot_id);
    },
    joinChannel: function(bot_id, channel) {

    },

    createBot: function(bot, channels)
    {
      bot.channels = channels.split(",");
      $http({
        method: 'POST',
        url: '/connect/',
        data: JSON.stringify(bot),
        headers: {
          'Content-Type': 'application/json'
        }
      })
    },

    searchFile: function(query, start)
    {
      return $http({
        method: 'POST',
        url: '/search/',
        data: JSON.stringify({query: query, start: start}),
        headers: {
          'Content-Type': 'application/json'
        }
      }).then(function(result) {
        return result.data;
      });
    },

    requestFile: function(bot_id, nick, slot) {
      $http({
        method: 'POST',
        url: '/bot/' + bot_id + '/request',
        data: JSON.stringify({ nick: nick, slot: slot }),
        headers: {
          'Content-Type': 'application/json'
      }})
    },

    removeFileFromList: function(bot_id, file_id) {
      $http({
        method: 'POST',
        url: '/bot/' + bot_id + '/delete/' + file_id,
        headers: {
          'Content-Type': 'application/json'
        }})
    },
    cancelDownloadFromList: function(bot_id, file_id) {
      $http({
        method: 'POST',
        url: '/bot/' + bot_id + '/cancel/' + file_id,
        headers: {
          'Content-Type': 'application/json'
        }})
    },
  }
});

app.factory('sharedDataService' , function () {
     var download_stat = {
        total_downloads_count: 0,
        total_bps: 0,

        active_downloads: [],
        finished_downloads: []
    };
    return download_stat;
});

app.filter('secondsToDateTime', function() {
    return function(seconds) {
        var d = new Date(0,0,0,0,0,0,0);
        d.setSeconds(seconds);
        return d;
    };
});

app.controller('StatusCtrl', function($scope, $timeout, $interval, apiService, sharedDataService) {
  $scope.download_stat = sharedDataService;

  $scope.getBots = function(){
    apiService.getBots().then(function(data){
      console.log(data);

      var active_downloads = [];
      var finished_downloads = [];
      var total_bps = 0;

      $scope.total_size = 0;
      $scope.total_announces = 0;

      var bots = data["bots"];
      for (var i = 0; i < bots.length; i++) {
        $scope.total_size += bots[i].total_size;
        $scope.total_announces += bots[i].announces;
      }

      var downloads = data["downloads"];
      for (var i = 0; i < downloads.length; i++) {
        var dl = downloads[i];
        total_bps += dl.bytes_per_second;
        dl["time_left"] = (dl.size - dl.received) / dl.bytes_per_second;
        dl["received_percent"] = dl.size / dl.received * 100.0;

        active_downloads.push(dl);
      }

      $scope.download_stat.active_downloads = active_downloads;
      $scope.download_stat.total_bps = total_bps;
      $scope.download_stat.finished_downloads = finished_downloads;
      $scope.download_stat.total_downloads_count = active_downloads.length + finished_downloads.length;
    });
  };

  $scope.cancelDownload = function(file)
  {
    apiService.cancelDownloadFromList(file.bot.id, file.id);
    $scope.getBots();
  };

  $scope.removeFromList = function(file)
  {
    apiService.removeFileFromList(file.bot.id, file.id);
    $scope.getBots();
  };

  $scope.removeFromDisk = function(file)
  {
    console.log("To be implemented");
  };

  $scope.intervalFunction = function()
  {
    $interval(function(){ $scope.getBots() }, 3000)
  };

  $scope.getBots();

  // Kick off the interval
  $scope.intervalFunction();
});

app.controller('BotContextCtrl', function($scope, $uibModal, apiService, sharedDataService) {
  $scope.download_stat = sharedDataService;
  $scope.openChannelModal = function(bot)
  {
    var modalInstance = $uibModal.open({
      animation: true,
      templateUrl: 'joinChannel.html',
      controller: 'JoinChannelModalCtrl',
      resolve: {
        bot: function() { return bot || null; }
      }
    });

    modalInstance.result.then(function(bot) {
      $scope.bot = bot;
    });
  };

  $scope.openFileRequestModal = function(bot)
  {
    var modalInstance = $uibModal.open({
      animation: true,
      templateUrl: 'requestFile.html',
      controller: 'FileRequestModalCtrl',
      resolve: {
        bot: function() { return bot || null; }
      }
    });

    modalInstance.result.then(function(bot) {
      $scope.bot = bot;
    });
  };

  $scope.on_launch = function(bot, channels)
  {
    apiService.createBot(bot, channels);
  };

  $scope.disconnect = function(bot)
  {
    apiService.disconnect(bot.id);
  };
});

app.controller('SearchCtrl', function($scope, apiService){
  $scope.search = [];
  $scope.loading = false;

  $scope.on_search = function(query, start) {
    $scope.loading = true;
    apiService.searchFile(query, start).then(function(result) { $scope.search = result; $scope.loading = false; });
  };

  $scope.add_download = function(request) {
    apiService.requestFile(request.bot_id, request.bot, request.slot);
  };

  $scope.get_paginations = function(results) {
    if(results != null)
    {
      return new Array(Math.floor(results/25) + ((results % 25) > 0));
    }
  };
});

app.controller('VideoContextCtrl', function($scope, $uibModal, apiService){
  $scope.openVideoModal = function(video)
  {
    var modalInstance = $uibModal.open({
      animation: true,
      templateUrl: 'playVideo.html',
      controller: 'PlayVideoModalCtrl',
      resolve: {
        video: function() { return video || null; }
      }
    });

    modalInstance.result.then(function(video) {
      $scope.video = video;
    });
  }
});

app.controller('JoinChannelModalCtrl', function ($scope, $uibModalInstance, apiService, bot) {
  $scope.bot = bot;

  $scope.on_submit = function () {
    apiService.joinChannel($scope.channel);
  };

  $scope.ok = function () {
    $uibModalInstance.close();
  };

  $scope.cancel = function () {
    $uibModalInstance.dismiss('cancel');
  };
});

app.controller('FileRequestModalCtrl', function ($scope, $uibModalInstance, apiService, bot) {
  $scope.bot = bot;

  $scope.on_submit = function () {
    apiService.requestFile(bot.id, $scope.nick, $scope.slot);
  };

  $scope.ok = function () {
    $uibModalInstance.close();
  };

  $scope.cancel = function () {
    $uibModalInstance.dismiss('cancel');
  };
});

app.controller('PlayVideoModalCtrl', function ($scope, $uibModalInstance, apiService, video) {
  $scope.video_src = video;
  console.log($scope.video_src);
  $scope.on_submit = function () {
    //apiService.requestFile(bot.id, $scope.nick, $scope.slot);
  };

  $scope.ok = function () {
    $uibModalInstance.close();
  };

  $scope.cancel = function () {
    $uibModalInstance.dismiss('cancel');
  };
});
