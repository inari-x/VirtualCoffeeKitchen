from flask import Flask
import requests
import datetime
import os
import jwt

app = Flask(__name__)

def get_zoom_token():
    return os.environ['ZOOM_TOKEN']

@app.route('/')
def hello_world():
  return 'Hello, Docker!'


@app.route('/participant-count')
def participant_count():
  return '42'

@app.route('/create-meeting')
def zoom_api():
  j = {
   "settings": {
    "join_before_host": True
    }}
  r = requests.post(f"https://api.zoom.us/v2/users/larapour@googlemail.com/meetings", headers = {
    'authorization': f"bearer {get_zoom_token()}", 'content-type': 'application/json'}, json = j)
  return r.json().get('join_url')


if __name__ == '__main__':
  app.run(host='::', debug=True)