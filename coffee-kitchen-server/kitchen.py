from flask import Flask, request
import requests
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

def post_chat_message(recipient_email, tech_user):
  jsn = {
    'message': f"This is your coffee kitchen join URL: https://us06web.zoom.us/j/5024947364?pwd=OUw2ZnE3VFFXcjVZKzJkdHJidXF2Zz09",
    'to_contact': f"{recipient_email}"
  }
  post_response = post_request(jsn, (f"chat/users/{tech_user}/messages"))
  return post_response

def participant_count(meeting_id):
  r = requests.get(f"https://api.zoom.us/v2/metrics/meetings", headers = {
    'authorization': f"bearer {get_zoom_token()}"})
  """participants =  r.json().get('participants')
  participant_number = 0
  for p in participants:
    if 'leave_time' in p:
      participant_number += 1"""
  json = r.json()
  meeting = json.get('meetings')
  participants = meeting[0].get('participants')
  return str(participants)

@app.route('/')
def hello_world():
  return 'Hello, Docker!'


@app.route('/participant-count')
def count_participants():
  return participant_count('5024947364')

@app.route('/join-meeting', methods=['POST'])
def zoom_api():
  recipient_email = request.form.get('email')
  tech_user = os.environ['TECH_USER']
  chat_message_response = post_chat_message(recipient_email, tech_user)
  return chat_message_response.json()

if __name__ == '__main__':
  app.run(host='::', debug=True, port=80)