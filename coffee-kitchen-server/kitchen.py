from flask import Flask, request
from datetime import datetime
import requests
import datetime
import os
import jwt
import base64

app = Flask(__name__)

def get_zoom_token():
  header = 'Basic ' + base64.b64encode(f"{os.environ['CLIENT_ID']}:{os.environ['CLIENT_SECRET']}".encode()).decode()
  url = 'https://zoom.us/oauth/token'
  headers = {
    'Host': 'zoom.us',
    'Authorization': header
  }
  data = {
    'grant_type': 'account_credentials',
    'account_id': f"{os.environ['ACCOUNT_ID']}"
  }
  r = requests.post(url, headers=headers, data=data)
  return r.json().get('access_token')

def post_request(jsn, api):
  r = requests.post(f"https://api.zoom.us/v2/{api}", headers = {
    'authorization': f"bearer {get_zoom_token()}", 'content-type': 'application/json'}, json = jsn)
  return r

def post_chat_message(join_url, recipient, recipient_email):
  jsn = {
    'message': f"This is your coffee kitchen join URL: {join_url}",
    'to_contact': f"{recipient_email}"
  }
  post_response = post_request(jsn, (f"chat/users/{recipient}/messages"))
  return post_response

"""def participant_count(meeting_id):
  r = requests.get(f"{https:/api.zoom.us/v2/metrics/meetings/{meeting_id}/participants)
  participants =  r.json().get('participants')
  participant_number = 0
  for each p in participants:
    if 'leave_time' in p:
      participant_number += 1
return participant_number"""

@app.route('/')
def hello_world():
  return 'Hello, Docker!'


"""@app.route('/participant-count')
  return participant_count()"""

@app.route('/create-meeting', methods=['POST'])
def zoom_api():
  tech_user = os.environ['TECH_USER']
  now = datetime.datetime.now()
  start_time = now.strftime("%Y-%m-%dT%H:%M:%SZ")
  recipient_email = request.form.get('email')

  jsn = {
    'topic': 'Coffee Kitchen',
    'type': '2',
    'start_time': f"{start_time}",
    'duration': '240',
    'timezone': 'Australia/Sydney',
    'password': f"{os.environ['ZOOM_PASSWORD']}",
    'agenda': 'Chat',
    'settings': {
      'join_before_host': True,
      'waiting_room': False
      }
    }
  r = post_request(jsn, f"users/{tech_user}/meetings")
  join_url = r.json().get('join_url')
  chat_message_response = post_chat_message(join_url, tech_user, recipient_email)
  return chat_message_response.json()

if __name__ == '__main__':
  app.run(host='::', debug=True, port=80)