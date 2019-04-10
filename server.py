import os
import sys
from flask import Flask, request, flash, redirect, render_template, send_file
app = Flask(__name__)

UPLOAD_FOLDER = './videos'

@app.route("/", methods=['GET'])
def landing():
	l = []
	for d, _, f in os.walk(UPLOAD_FOLDER):
		for fn in f:
			sys.stderr.write(str(fn))
			if fn[-3:] == '.in':
				l += [fn[:-3]]
	return render_template('main.html', projects=l)

@app.route("/upload/<s>", methods=['POST'])
def upload_file(s):
	if 'file' not in request.files:
		return redirect('/')
	file = request.files['file']
	if file.filename == '':
		return redirect('/')
	if file:
		file.save(os.path.join(UPLOAD_FOLDER, s + '.in'))
		os.remove(os.path.join(UPLOAD_FOLDER, s + '.out'))
		return redirect('/project/' + s)

@app.route("/project/<s>", methods=['GET'])
def project_page(s):
	hin = os.path.isfile(os.path.join(UPLOAD_FOLDER, s + '.in'))
	hout = os.path.isfile(os.path.join(UPLOAD_FOLDER, s + '.out'))
	return render_template('project.html', proj=s, has_in=hin, has_out=hout)

@app.route("/videos/<s>", methods=['GET'])
def get_uploaded_file(s):
	return send_file(os.path.join(UPLOAD_FOLDER, s))
