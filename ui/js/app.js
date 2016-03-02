var app = angular.module("streamApp", ['ui.bootstrap']);

app.factory('apiService', function($http) {
  return {
    getBots: function() {
      return $http.get('http://localhost:1984/status')
        .then(function(result) {
          return result.data;
        });
    },
    disconnect: function(bot_id) {
      $http.get('http://localhost:1984/disconnect/' + bot_id);
    },
    joinChannel: function(bot_id, channel) {

    },
    requestFile: function(bot_id, nick, slot) {
      $http({
        method: 'POST',
        url: 'http://localhost:1984/bot/' + bot_id + '/request',
        data: JSON.stringify({ nick: nick, slot: slot }),
        headers: {
          'Content-Type': 'application/json'
      }})
    }
  }
});

app.controller('StatusCtrl', function($scope, $timeout, apiService) {
  $scope.getBots = function(){
    apiService.getBots().then(function(bots) {
      $scope.bots = bots;
    });
  };
  // Function to replicate setInterval using $timeout service.
  $scope.intervalFunction = function(){
    $timeout(function() {
      $scope.getBots();
      $scope.intervalFunction();
    }, 3000)
  };

  $scope.getBots();
  // Kick off the interval
  $scope.intervalFunction();
});

app.controller('TabsDemoCtrl', function ($scope, $window) {
  $scope.view_tab = 'tab1';

  $scope.changeTab = function(tab) {
    $scope.view_tab = tab;
  }

  $scope.model = {
    name: 'Tabs'
  };
});

app.controller('BotContextCtrl', function($scope, $uibModal, apiService) {
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

  $scope.disconnect = function(bot)
  {
    apiService.disconnect(bot.id);
  }
});

app.controller('JoinChannelModalCtrl', function ($scope, $uibModalInstance, apiService, bot) {
  $scope.bot = bot;

  $scope.on_submit = function () {
    apiService.joinChannel($scope.channel);
  }

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