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
        all_downloads: 0,
        active_downloads: 0,
        finished_downloads: 0
    };
    return download_stat;
});

app.controller('StatusCtrl', function($scope, $timeout, $interval, apiService, sharedDataService) {
  $scope.download_stat = sharedDataService;

  $scope.getBots = function(){
    apiService.getBots().then(function(bots) {
      $scope.bots = bots;

      var downloads = [];
      for (var i = 0; i < bots.length; i++) {
        for (var j = 0; j < bots[i].downloads.length; j++) {
          var dl = bots[i].downloads[j];
          dl["bot"] = bots[i];
          downloads.push(dl);
        }
      }
      var active_downloads = 0;
      var finished_downloads = 0;
      for(var i = 0; i < downloads.length; i++) {
        if(downloads[i].state == 3)
        {
          finished_downloads += 1;
        }
        if(downloads[i].state == 2)
        {
          active_downloads += 1;
        }
      }

      $scope.downloads = downloads;
      $scope.download_stat.all_downloads = downloads.length;
      $scope.download_stat.active_downloads = active_downloads;
      $scope.download_stat.finished_downloads = finished_downloads;
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

  // Function to replicate setInterval using $timeout service.
  /*$scope.intervalFunction = function(){
    $timeout(function() {
      $scope.getBots();
      $scope.intervalFunction();
    }, 3000)
  };
  */
  $scope.intervalFunction = function()
  {
    $interval(function(){
    apiService.getBots().then(function(bots) {
      $scope.bots = bots;

      var downloads = [];
      for (var i = 0; i < bots.length; i++) {
        for (var j = 0; j < bots[i].downloads.length; j++) {
          var dl = bots[i].downloads[j];
          dl["bot"] = bots[i];
          downloads.push(dl);
        }
      }
      var active_downloads = 0;
      var finished_downloads = 0;
      for(var i = 0; i < downloads.length; i++) {
        if(downloads[i].state == 3)
        {
          finished_downloads += 1;
        }
        if(downloads[i].state == 2)
        {
          active_downloads += 1;
        }
      }

      $scope.downloads = downloads;
      $scope.download_stat.all_downloads = downloads.length;
      $scope.download_stat.active_downloads = active_downloads;
      $scope.download_stat.finished_downloads = finished_downloads;
    });
  },3000)
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
    apiService.searchFile(query, start).then(function(result) { $scope.search = result; $scope.loading = false; });;
  };
  
  $scope.add_download = function(request) {
    apiService.requestFile(request.bot_id, request.bot, request.slot);
  };

  $scope.get_paginations = function(results) {
    if(results != null)
    {
      x = new Array(Math.floor(results/25) + ((results % 25) > 0)); 
      console.log(x);
      return x;
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
