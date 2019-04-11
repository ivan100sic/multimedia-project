import os
import sys
import re
import subprocess
import random
from flask import Flask, request, flash, redirect, render_template, send_from_directory
app = Flask(__name__)

UPLOAD_FOLDER = './videos'
EFFECTS = {'gmajor' : 'G Major', 'test-effect': 'Test effect'}

def random_string(n):
	a = ""
	for x in range(n):
		a += str(random.randint(0, 9))
	return a

def name_ok(s):
	if re.match('^[a-zA-Z0-9_\\.]+$', s):
		return True
	else:
		return False

@app.route("/", methods=['GET'])
def landing():
	l = []
	for d, _, f in os.walk(UPLOAD_FOLDER):
		for fn in f:
			if fn[-3:] == '.in':
				l += [fn[:-3]]
	return render_template('main.html', projects=l)

@app.route("/upload/<s>", methods=['POST'])
def upload_file(s):
	if not name_ok(s):
		return redirect('/')
	if 'file' not in request.files:
		return redirect('/')
	file = request.files['file']
	if file.filename == '':
		return redirect('/')
	if file:
		file.save(os.path.join(UPLOAD_FOLDER, s + '.in'))
		if os.path.isfile(os.path.join(UPLOAD_FOLDER, s + '.out')):
			os.remove(os.path.join(UPLOAD_FOLDER, s + '.out'))
		return redirect('/project/' + s)
	else:
		return redirect('/')

@app.route("/project/<s>", methods=['GET'])
def project_page(s):
	if not name_ok(s):
		return redirect('/')
	hin = os.path.isfile(os.path.join(UPLOAD_FOLDER, s + '.in'))
	hout = os.path.isfile(os.path.join(UPLOAD_FOLDER, s + '.out'))
	return render_template('project.html', proj=s, has_in=hin, has_out=hout,
		effects=EFFECTS, rand=random_string(16))

@app.route("/videos/<s>", methods=['GET'])
def get_uploaded_file(s):
	if not name_ok(s):
		return redirect('/')
	return send_from_directory(UPLOAD_FOLDER, s[16:], attachment_filename=s)

@app.route('/apply/<e>/<s>', methods=['GET', 'POST']) # TODO remove GET
def apply_effect(s, e):
	if not name_ok(s):
		return redirect('/')
	if not e in EFFECTS:
		return redirect('/')
	os.remove(os.path.join(UPLOAD_FOLDER, s + '.out'))
	subprocess.Popen(['./'+e, os.path.join(UPLOAD_FOLDER, s)])
	return 'Started...'
	
@app.route('/check/<s>', methods=['POST'])
def check_if_done(s):
	if not name_ok(s):
		return redirect('/')
	if os.path.isfile(os.path.join(UPLOAD_FOLDER, s + '.out')):
		return '1'
	else:
		return '0'
