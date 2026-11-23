from flask import Flask

app = Flask(__name__)  # __name__ est le nom du programme

@app.route("/")  # ces 3 lignes sont une "view function"
def bienvenue():
    return "<h2>Bienvenue dans mon application!</h2>"