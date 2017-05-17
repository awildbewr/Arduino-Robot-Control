'use strict';

var watchify = require('watchify');
var browserify = require('browserify');
var vueify = require('vueify');
var babelify = require('babelify');
var gulp = require('gulp');
var source = require('vinyl-source-stream');
var buffer = require('vinyl-buffer');
var gutil = require('gulp-util');
var sourcemaps = require('gulp-sourcemaps');

const WebSocket = require('ws');
var wss;

gulp.task('watch', ['js'], function() {
  wss = new WebSocket.Server({ port: 1234 });
  console.log("WebSocket created");
  wss.on('connection', function connection(ws) {
    console.log("Got connection");
  });

  gulp.watch('src/**/*', ['ws-update']);
});

var entries = ['./src/app.js'];
if (process.env.NODE_ENV != "production") {
  entries.push('./src/dev-reload.js');
}

var bundler = browserify({
  // Required watchify args
  cache: {}, packageCache: {}, fullPaths: true,
  // Browserify Options
  entries: entries,
  debug: true,
  transform: [vueify, babelify.configure({presets: ["es2015"]})]
});

gulp.task('js', function() {
  var bundle = function() {
    return bundler
      .bundle()
      .on('error', gutil.log)
      .pipe(source('app.js'))
      .pipe(buffer())
      // optional, remove if you dont want sourcemaps
      .pipe(sourcemaps.init({loadMaps: true})) // loads map from browserify file
      // Add transformation tasks to the pipeline here.
      .pipe(sourcemaps.write('./')) // writes .map file
      .pipe(gulp.dest('./dist'));
  };

  if(global.isWatching) {
    bundler = watchify(bundler);
    bundler.on('update', bundle);
  }

  return bundle();
});

gulp.task('ws-update', ['js'], function() {
  wss.clients.forEach(function each(client) {
    if (client.readyState === WebSocket.OPEN) {
      client.send("reload");
    }
  });
});