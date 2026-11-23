import json
from datetime import datetime
from pathlib import Path
from flask import Flask

app = Flask(__name__)

DOSSIER = Path(__file__).resolve().parent


@app.route("/")
def accueil():
    return "Coffre-fort !"


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