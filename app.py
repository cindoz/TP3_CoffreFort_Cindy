import json
from datetime import datetime
from pathlib import Path
from flask import Flask, render_template, request, redirect, url_for

app = Flask(__name__)

DOSSIER = Path(__file__).resolve().parent

def lire_json(nom_fichier):
    with open(DOSSIER / nom_fichier, "r", encoding="utf-8") as fichier:
        return json.load(fichier)

def enregistrer_json(nom_fichier, donnees):
    with open(DOSSIER / nom_fichier, "w", encoding="utf-8") as fichier:
        json.dump(donnees, fichier, indent=4)

def authentifier(numero, mot_de_passe):
    utilisateurs = lire_json("utilisateurs.json")

    autorise = False

    for utilisateur in utilisateurs:
        if utilisateur["numero"] == numero and utilisateur["mot_de_passe"] == mot_de_passe:
            autorise = True
            break

    historique = lire_json("historique.json")

    historique.append({
        "numero": numero,
        "date": datetime.now().isoformat(timespec="seconds"),
        "resultat": "AUTORISE" if autorise else "REFUSE"
    })

    enregistrer_json("historique.json", historique)

    return autorise

@app.route("/")
def accueil():
    return "Coffre-fort !"

@app.route("/utilisateurs")
def afficher_utilisateurs():
    utilisateurs = lire_json("utilisateurs.json")
    return render_template("utilisateurs.html", utilisateurs=utilisateurs)


@app.route("/utilisateurs/ajouter", methods=["GET", "POST"])
def add_utilisateur():
    if request.method == "POST":
        numero = request.form["numero"]
        mot_de_passe = request.form["mot_de_passe"]

        utilisateurs = lire_json("utilisateurs.json")

        if (numero.isascii() and numero.isdigit()
                and mot_de_passe.isascii() and mot_de_passe.isdigit()
                and 4 <= len(numero) <= 6
                and 4 <= len(mot_de_passe) <= 6):

            for utilisateur in utilisateurs:
                if utilisateur["numero"] == numero:
                    return "Ce numéro existe déjà."

            utilisateurs.append({
                "numero": numero,
                "mot_de_passe": mot_de_passe
            })

            enregistrer_json("utilisateurs.json", utilisateurs)

            return redirect(url_for("afficher_utilisateurs"))

        return "Le numéro et le mot de passe doivent contenir 4 à 6 chiffres."

    return render_template("add_utilisateur.html")

    if request.method == "POST":
        numero = request.form["numero"]
        mot_de_passe = request.form["mot_de_passe"]

        utilisateurs = lire_json("utilisateurs.json")
        utilisateurs.append({"numero": numero, "mot_de_passe": mot_de_passe})
        enregistrer_json("utilisateurs.json", utilisateurs)

        return redirect(url_for('afficher_utilisateurs'))

    return render_template("add_utilisateur.html")

if __name__ == "__main__":
    app.run()
