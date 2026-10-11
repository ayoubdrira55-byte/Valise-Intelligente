// =====================================================================
//  Valise intelligente connectée — application Flutter (version démo)
//
//  Fichier unique, sans dépendance externe : les données de la valise sont
//  simulées localement pour pouvoir présenter l'application sans carte ESP32.
//  Dans la version connectée, ces données viennent de Firebase Realtime
//  Database (/valise/etat, /valise/historique, /valise/commandes).
//
//  Lancer :  flutter create valise_app
//            remplacer lib/main.dart par ce fichier
//            flutter run
// =====================================================================
import 'dart:async';
import 'dart:math';

import 'package:flutter/material.dart';

void main() => runApp(const ValiseApp());

const Color kBleuNuit = Color(0xFF12324F);
const Color kVert = Color(0xFF1BA672);
const Color kRouge = Color(0xFFD64545);
const Color kJaune = Color(0xFFE8A317);

class ValiseApp extends StatelessWidget {
  const ValiseApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Valise connectée',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(colorSchemeSeed: kBleuNuit, useMaterial3: true),
      home: const Accueil(),
    );
  }
}

// ---------------------------------------------------------------------
// Modèle : état de la valise (simulé)
// ---------------------------------------------------------------------
enum EtatValise { desarmee, armee, alarme }

class Evenement {
  Evenement(this.texte, this.couleur, this.icone) : heure = DateTime.now();
  final String texte;
  final Color couleur;
  final IconData icone;
  final DateTime heure;
}

class ValiseModel extends ChangeNotifier {
  ValiseModel() {
    _timer = Timer.periodic(const Duration(seconds: 1), (_) => _tick());
    _log('Application démarrée', kBleuNuit, Icons.power_settings_new);
  }

  static const double latSfax = 34.7406, lonSfax = 10.7603;
  final Random _rnd = Random();
  late final Timer _timer;

  EtatValise etat = EtatValise.desarmee;
  double lat = latSfax, lon = lonSfax;
  double batterie = 82;
  bool chargeQi = true;
  final List<Evenement> historique = [];

  void _tick() {
    lat = latSfax + (_rnd.nextDouble() - 0.5) * 0.001;
    lon = lonSfax + (_rnd.nextDouble() - 0.5) * 0.001;
    batterie = (batterie + (chargeQi ? -0.15 : 0.02)).clamp(5, 100).toDouble();
    notifyListeners();
  }

  void _log(String texte, Color c, IconData i) {
    historique.insert(0, Evenement(texte, c, i));
  }

  void setArmee(bool armee) {
    etat = armee ? EtatValise.armee : EtatValise.desarmee;
    _log(armee ? 'Valise armée' : 'Valise désarmée', armee ? kVert : kBleuNuit,
        armee ? Icons.lock : Icons.lock_open);
    notifyListeners();
  }

  void simulerIntrusion(String cause) {
    if (etat != EtatValise.armee) return;
    etat = EtatValise.alarme;
    _log('ALARME : $cause', kRouge, Icons.warning_amber);
    notifyListeners();
  }

  void setChargeQi(bool on) {
    chargeQi = on;
    _log(on ? 'Recharge sans fil activée' : 'Recharge sans fil coupée', kJaune, Icons.bolt);
    notifyListeners();
  }

  @override
  void dispose() {
    _timer.cancel();
    super.dispose();
  }
}

// ---------------------------------------------------------------------
// Écran principal avec navigation par onglets
// ---------------------------------------------------------------------
class Accueil extends StatefulWidget {
  const Accueil({super.key});

  @override
  State<Accueil> createState() => _AccueilState();
}

class _AccueilState extends State<Accueil> {
  final ValiseModel model = ValiseModel();
  int onglet = 0;

  @override
  void dispose() {
    model.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: model,
      builder: (context, _) {
        final pages = [
          CarteView(model: model),
          SecuriteView(model: model),
          EnergieView(model: model),
          HistoriqueView(model: model),
        ];
        final alarme = model.etat == EtatValise.alarme;
        return Scaffold(
          appBar: AppBar(
            backgroundColor: alarme ? kRouge : kBleuNuit,
            foregroundColor: Colors.white,
            title: const Text('Ma valise connectée'),
            actions: [
              Padding(
                padding: const EdgeInsets.only(right: 16),
                child: Row(children: [
                  const Icon(Icons.battery_std, size: 20),
                  const SizedBox(width: 4),
                  Text('${model.batterie.round()} %'),
                ]),
              ),
            ],
          ),
          body: Padding(padding: const EdgeInsets.all(16), child: pages[onglet]),
          bottomNavigationBar: NavigationBar(
            selectedIndex: onglet,
            onDestinationSelected: (i) => setState(() => onglet = i),
            destinations: const [
              NavigationDestination(icon: Icon(Icons.map_outlined), label: 'Carte'),
              NavigationDestination(icon: Icon(Icons.shield_outlined), label: 'Sécurité'),
              NavigationDestination(icon: Icon(Icons.bolt_outlined), label: 'Énergie'),
              NavigationDestination(icon: Icon(Icons.history), label: 'Historique'),
            ],
          ),
        );
      },
    );
  }
}

// ---------------------------------------------------------------------
// Onglet Carte : position GPS (carte schématique dessinée)
// ---------------------------------------------------------------------
class CarteView extends StatelessWidget {
  const CarteView({super.key, required this.model});
  final ValiseModel model;

  @override
  Widget build(BuildContext context) {
    return Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
      Text('Position de la valise', style: Theme.of(context).textTheme.titleLarge),
      const SizedBox(height: 4),
      const Text('Sfax, Tunisie — position GPS simulée'),
      const SizedBox(height: 16),
      Expanded(
        child: ClipRRect(
          borderRadius: BorderRadius.circular(16),
          child: CustomPaint(
            painter: CartePainter(
              dx: (model.lon - ValiseModel.lonSfax) / 0.001,
              dy: (model.lat - ValiseModel.latSfax) / 0.001,
              alarme: model.etat == EtatValise.alarme,
            ),
            child: const SizedBox.expand(),
          ),
        ),
      ),
      const SizedBox(height: 12),
      Text('Lat : ${model.lat.toStringAsFixed(5)}   Lon : ${model.lon.toStringAsFixed(5)}'),
    ]);
  }
}

class CartePainter extends CustomPainter {
  CartePainter({required this.dx, required this.dy, required this.alarme});
  final double dx, dy;
  final bool alarme;

  @override
  void paint(Canvas canvas, Size size) {
    canvas.drawRect(Offset.zero & size, Paint()..color = const Color(0xFFE9EEF2));
    final rue = Paint()
      ..color = Colors.white
      ..strokeWidth = 10;
    for (double x = 30; x < size.width; x += 90) {
      canvas.drawLine(Offset(x, 0), Offset(x, size.height), rue);
    }
    for (double y = 40; y < size.height; y += 110) {
      canvas.drawLine(Offset(0, y), Offset(size.width, y), rue);
    }
    final c = Offset(size.width / 2 + dx * size.width * 0.4, size.height / 2 - dy * size.height * 0.4);
    final couleur = alarme ? kRouge : kBleuNuit;
    canvas.drawCircle(c, 40, Paint()..color = couleur.withOpacity(0.15));
    canvas.drawCircle(c, 12, Paint()..color = couleur);
    canvas.drawCircle(c, 5, Paint()..color = Colors.white);
  }

  @override
  bool shouldRepaint(CartePainter old) => old.dx != dx || old.dy != dy || old.alarme != alarme;
}

// ---------------------------------------------------------------------
// Onglet Sécurité : armement et simulation d'intrusion
// ---------------------------------------------------------------------
class SecuriteView extends StatelessWidget {
  const SecuriteView({super.key, required this.model});
  final ValiseModel model;

  @override
  Widget build(BuildContext context) {
    final etat = model.etat;
    final (texte, couleur, icone) = switch (etat) {
      EtatValise.desarmee => ('Désarmée', Colors.grey, Icons.lock_open),
      EtatValise.armee => ('Armée — surveillance active', kVert, Icons.lock),
      EtatValise.alarme => ('ALARME DÉCLENCHÉE', kRouge, Icons.warning_amber),
    };
    return ListView(children: [
      Card(
        color: couleur.withOpacity(0.12),
        child: Padding(
          padding: const EdgeInsets.all(24),
          child: Column(children: [
            Icon(icone, size: 64, color: couleur),
            const SizedBox(height: 8),
            Text(texte, style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold, color: couleur)),
          ]),
        ),
      ),
      const SizedBox(height: 12),
      SwitchListTile(
        title: const Text('Armer la valise'),
        subtitle: const Text('Surveille l\'ouverture et les mouvements'),
        value: etat != EtatValise.desarmee,
        onChanged: model.setArmee,
      ),
      const Divider(),
      const Padding(
        padding: EdgeInsets.symmetric(vertical: 8),
        child: Text('Simulation (démo)', style: TextStyle(fontWeight: FontWeight.bold)),
      ),
      Row(children: [
        Expanded(
          child: OutlinedButton.icon(
            icon: const Icon(Icons.sensor_door_outlined),
            label: const Text('Ouverture'),
            onPressed: etat == EtatValise.armee ? () => model.simulerIntrusion('ouverture') : null,
          ),
        ),
        const SizedBox(width: 12),
        Expanded(
          child: OutlinedButton.icon(
            icon: const Icon(Icons.vibration),
            label: const Text('Mouvement'),
            onPressed: etat == EtatValise.armee ? () => model.simulerIntrusion('mouvement') : null,
          ),
        ),
      ]),
    ]);
  }
}

// ---------------------------------------------------------------------
// Onglet Énergie : batterie et recharge sans fil
// ---------------------------------------------------------------------
class EnergieView extends StatelessWidget {
  const EnergieView({super.key, required this.model});
  final ValiseModel model;

  @override
  Widget build(BuildContext context) {
    final b = model.batterie;
    final couleur = b > 50 ? kVert : (b > 20 ? kJaune : kRouge);
    return ListView(children: [
      Text('Batterie de la valise', style: Theme.of(context).textTheme.titleLarge),
      const SizedBox(height: 16),
      ClipRRect(
        borderRadius: BorderRadius.circular(12),
        child: LinearProgressIndicator(value: b / 100, minHeight: 28, color: couleur),
      ),
      const SizedBox(height: 8),
      Text('${b.toStringAsFixed(1)} %  ${model.chargeQi ? "— en décharge (recharge Qi active)" : "— en charge"}'),
      const SizedBox(height: 24),
      SwitchListTile(
        secondary: Icon(Icons.bolt, color: model.chargeQi ? kJaune : Colors.grey),
        title: const Text('Recharge sans fil (Qi)'),
        subtitle: const Text('Recharge un téléphone posé sur la valise'),
        value: model.chargeQi,
        onChanged: model.setChargeQi,
      ),
    ]);
  }
}

// ---------------------------------------------------------------------
// Onglet Historique : journal des événements
// ---------------------------------------------------------------------
class HistoriqueView extends StatelessWidget {
  const HistoriqueView({super.key, required this.model});
  final ValiseModel model;

  @override
  Widget build(BuildContext context) {
    String hh(DateTime d) =>
        '${d.hour.toString().padLeft(2, '0')}:${d.minute.toString().padLeft(2, '0')}:${d.second.toString().padLeft(2, '0')}';
    return ListView.separated(
      itemCount: model.historique.length,
      separatorBuilder: (_, __) => const Divider(height: 1),
      itemBuilder: (_, i) {
        final e = model.historique[i];
        return ListTile(
          leading: CircleAvatar(backgroundColor: e.couleur, child: Icon(e.icone, color: Colors.white, size: 20)),
          title: Text(e.texte),
          trailing: Text(hh(e.heure)),
        );
      },
    );
  }
}
