import asyncio

from bleak import BleakClient, BleakScanner

# ---- Configuration Bluetooth ----
NOM_ESP32 = "CoffreESP32"

UUID_REQUETE = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"
UUID_REPONSE = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"

# ---- Vérification du format ----
def identifiants_valides(numero, mot_de_passe):
    numero_valide = (
        numero.isascii()
        and numero.isdigit()
        and 4 <= len(numero) <= 6
    )

    mot_de_passe_valide = (
        mot_de_passe.isascii()
        and mot_de_passe.isdigit()
        and 4 <= len(mot_de_passe) <= 6
    )

    return numero_valide and mot_de_passe_valide


# ---- Authentification ----
def verifier_demande(message):

    parties = message.split("|")

    if len(parties) != 3:
        print("Format de demande invalide")
        return None

    id_demande, numero, mot_de_passe = parties

    if not id_demande.isascii() or not id_demande.isdigit():
        print("Numero de demande invalide")
        return None

    if identifiants_valides(numero, mot_de_passe):
        from app import authentifier
        autorise = authentifier(numero, mot_de_passe)
    else:
        autorise = False

    if autorise:
        print("Acces autorise pour", numero)
        resultat = "1"
    else:
        print("Acces refuse pour", numero)
        resultat = "0"

    return id_demande + "|" + resultat


# ---- Communication Bluetooth ----
async def communiquer_avec_esp32():

    while True:
        try:
            print("Recherche de l'ESP32...")

            appareil = await BleakScanner.find_device_by_name(
                NOM_ESP32,
                timeout=10.0
            )

            if appareil is None:
                print("ESP32 introuvable")
                await asyncio.sleep(3)
                continue

            async with BleakClient(appareil) as client:
                print("ESP32 connecte !")

                demandes = asyncio.Queue()

                def recevoir_demande(_, donnees):
                    demandes.put_nowait(bytes(donnees))

                await client.start_notify(
                    UUID_REQUETE,
                    recevoir_demande
                )

                print("En attente d'une saisie...")

                while client.is_connected:
                    try:
                        donnees = await asyncio.wait_for(
                            demandes.get(),
                            timeout=1.0
                        )

                    except asyncio.TimeoutError:
                        continue

                    try:
                        message = donnees.decode("utf-8")
                        reponse = verifier_demande(message)

                    except UnicodeError:
                        print("Message impossible a lire")
                        continue

                    if reponse is not None:
                        await client.write_gatt_char(
                            UUID_REPONSE,
                            reponse.encode("utf-8"),
                            response=True
                        )

        except Exception as erreur:
            print("Erreur Bluetooth :", erreur)

        print("Nouvelle tentative de connexion...")
        await asyncio.sleep(3)

if __name__ == "__main__":
    asyncio.run(communiquer_avec_esp32())