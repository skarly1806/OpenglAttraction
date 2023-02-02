# Projet
## Description
Il s'agit d'un petit terrain avec un circuit de rollercoaster. Le joueur peut s'y déplacer, monter dans le wagon, et oberver le mouvement.

## Lancement
### Pour lancer le projet
* Pour créer le dossier build et faire les cmake et make initiaux, puis lancer le projet
```
./setup_all.sh
./bin/Projet_exe
```
* Pour build puis lancer le projet (utile apres avoir modifié le code)
```
. launch.sh
```

### Depuis wsl avec XLaunch (si besoin)

* Pour authoriser wsl à ouvrir des fenetres (avec XLaunch)
```
. startup.sh
```
## Manuel d'utilisation
|Touche|Effet|
|------|-----|
|Echap|Ferme la fenetre|
|Entrée|Change de caméra|
|Espace|Lance / arrête le wagon|
|Z|Avance|
|S|Recule|
|Q|Va a gauche (1ère personne seulement)|
|D|Va a droite (1ère personne seulement)|
|E|Monte dans le wagon (1ère personne seulement)|
