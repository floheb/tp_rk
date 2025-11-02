#!/bin/bash
# Script de nettoyage complet du rootkit
# Usage: bash clean.sh

echo "[*] Nettoyage du rootkit en cours..."

# 1. Tuer tous les processus backdoor
echo "[+] Arrêt des processus..."
pkill -f "systemd-grammar" 2>/dev/null
pkill -f "43459" 2>/dev/null
sleep 1

# 2. Supprimer le dossier d'installation
echo "[+] Suppression des fichiers..."
rm -rf /var/tmp/.systemd-grammar 2>/dev/null
rm -rf /tmp/.systemd-grammar 2>/dev/null

# 3. Nettoyer les alias dans .bashrc
if [ -f "$HOME/.bashrc" ]; then
    echo "[+] Nettoyage de .bashrc..."
    sed -i '/grammar/d' "$HOME/.bashrc" 2>/dev/null
    sed -i '/System grammar configuration/d' "$HOME/.bashrc" 2>/dev/null
fi

# 4. Nettoyer les alias dans .zshrc
if [ -f "$HOME/.zshrc" ]; then
    echo "[+] Nettoyage de .zshrc..."
    sed -i '/grammar/d' "$HOME/.zshrc" 2>/dev/null
    sed -i '/System grammar configuration/d' "$HOME/.zshrc" 2>/dev/null
fi

# 5. Nettoyer crontab
echo "[+] Nettoyage de crontab..."
crontab -l 2>/dev/null | grep -v "systemd-grammar" | grep -v "43459" | crontab - 2>/dev/null

# 6. Vérifications finales
echo ""
echo "[*] Vérifications finales..."

# Vérifier les processus
PROC_CHECK=$(ps aux | grep -E "systemd-grammar|43459" | grep -v grep)
if [ -z "$PROC_CHECK" ]; then
    echo "[✓] Aucun processus backdoor trouvé"
else
    echo "[!] Processus encore actifs:"
    echo "$PROC_CHECK"
fi

# Vérifier le port
PORT_CHECK=$(ss -tulpn 2>/dev/null | grep 43459)
if [ -z "$PORT_CHECK" ]; then
    echo "[✓] Port 43459 libre"
else
    echo "[!] Port encore en écoute:"
    echo "$PORT_CHECK"
fi

# Vérifier les fichiers
if [ ! -d "/var/tmp/.systemd-grammar" ] && [ ! -d "/tmp/.systemd-grammar" ]; then
    echo "[✓] Dossiers supprimés"
else
    echo "[!] Dossiers encore présents"
fi

# Vérifier crontab
CRON_CHECK=$(crontab -l 2>/dev/null | grep -E "systemd-grammar|43459")
if [ -z "$CRON_CHECK" ]; then
    echo "[✓] Crontab nettoyé"
else
    echo "[!] Tâches cron encore présentes:"
    echo "$CRON_CHECK"
fi

echo ""
echo "[*] Nettoyage terminé!"
echo "[*] Redémarrez votre terminal pour appliquer les changements des alias"
