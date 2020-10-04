#include "gameScreen.h"

#include "../Graphics/graphics.h"
#include "../Graphics/images.h"
#include "../Graphics/spineGfx.h"
#include "../Graphics/camera.h"
#include "../Graphics/debugRendering.h"
#include "../Graphics/imageSheets.h"
#include "../UI/text.h"
#include "../System/platformLog.h"
#include "../System/random.h"
#include "../Utils/helpers.h"
#include "../Utils/stretchyBuffer.h"
#include "../Components/generalComponents.h"
#include "../Processes/generalProcesses.h"
#include "../tween.h"
#include "../Graphics/color.h"
#include "../Utils/sequence.h"
#include "../collisionDetection.h"
#include "../sound.h"

#include "../System/ECPS/entityComponentProcessSystem.h"

static ECPS gameECPS;

static int blockSound;
static int breakSound;
static int deathSound;
static int* sbRoarSounds = NULL;

static int droneStrm;
static int doomStrm;
static float rumbleVolume;
static float desiredRumbleVolume;

static int youWinImg;
static EntityID youWinDisplay = INVALID_ENTITY_ID;

static int tutorialImg;
static EntityID tutorialDisplay = INVALID_ENTITY_ID;
static EntityID tutorialBG = INVALID_ENTITY_ID;

static bool ignoreTime = false;

static void createEyes( void );

typedef struct {
	float time;
	bool capitalize;
	bool replace;
} Stage;

static const char* threeCharWords[] = {
	"aer", "ago", "ait", "alo", "amo", "ara", "aro", "ars", "arx", "aut", "bis", "bos", "ire", "cui", "cur", "diu", "duo", "dux", "edo", "edi",
	"ego", "emo", "emi", "ire", "fas", "est", "fio", "hac", "hae", "hec", "has", "hic", "hec", "his", "hoc", "hos", "iam", "ibi", "nae", "ire",
	"ira", "ita", "ius", "lip", "lac", "leo", "lex", "lux", "mei", "mel", "mos", "mox", "mus", "dum", "neo", "nec", "non", "nos", "nox", "ire",
	"ops", "oro", "par", "pax", "nus", "per", "pes", "ire", "pia", "pre", "num", "pro", "qua", "que", "rem", "est", "qui", "que", "qui", "quo",
	"res", "rei", "res", "rex", "eum", "ruo", "rui", "rus", "sal", "sed", "sic", "etc", "seu", "sol", "sto", "sub", "sui", "sum", "fui", "sua",
	"tam", "ter", "tot", "tui", "tum", "ubi", "una", "uti", "vae", "vel", "ver", "via", "vir", "vis", "vix", "vos", "vox", 
};

static const char* fourCharWords[] = {
	"abeo", "acer", "acsi", "addo", "adeo", "eger", "egre", "ager", "alii", "alia", "alos", "amor", "ante", "apto", "apud", "aqua", "arca", "arma",
	"armo", "arto", "ater", "atra", "bene", "bibo", "cado", "cavi", "cedo", "celo", "cena", "ceno", "cibo", "cito", "clam", "cogo", "colo", "coma",
	"cras", "creo", "crur", "crux", "cubo", "cura", "curo", "demo", "dens", "deus", "dico", "dido", "dies", "diei", "dito", "dare", "dedi", "duco",
	"duro", "egeo", "eluo", "enim", "itum", "erga", "ergo", "erro", "eruo", "etsi", "oris", "fama", "fere", "fero", "tuli", "flax", "fleo", "fluo",
	"fore", "fors", "fovi", "esse", "frux", "fuga", "fugo", "furs", "gemo", "gens", "gero", "haec", "hanc", "haud", "haec", "hinc", "hora", "huic",
	"humo", "hunc", "idem", "ideo", "illa", "ille", "illi", "illo", "immo", "inda", "inde", "indo", "ioco", "ipse", "ipsa", "iste", "ista", "iter",
	"iuge", "iuro", "iuvo", "labo", "ledo", "leto", "leve", "lama", "eris", "laus", "lego", "leno", "lens", "leto", "levo", "ligo", "lima", "lino",
	"loci", "loco", "ludo", "lusi", "lues", "luna", "malo", "mane", "mare", "mens", "meus", "mica", "mihi", "mire", "miro", "misi", "modo", "fero",
	"mons", "mora", "mors", "moti", "plus", "muto", "neco", "nego", "nemo", "nisi", "nolo", "nota", "novo", "nunc", "odio", "olim", "onus", "opes",
	"opto", "opus", "ordo", "orno", "ovis", "pala", "paro", "pars", "gero", "peto", "pium", "pica", "pius", "pluo", "pono", "pons", "post", "prae",
	"quam", "prae", "prex", "puer", "puga", "pyga", "pupa", "pyus", "quae", "quam", "quas", "quem", "queo", "quae", "quod", "quia", "quid", "quin",
	"quod", "quos", "quot", "vera", "rego", "rexi", "repo", "reus", "rgis", "risi", "rogo", "rota", "sepe", "sane", "sano", "sato", "scio", "gero",
	"quam", "sedi", "sedo", "sero", "sese", "sine", "sino", "sive", "sono", "spes", "sumo", "suus", "suum", "quam", "tego", "texi", "dare", "tero",
	"texo", "tibi", "tres", "tria", "tunc", "unde", "unus", "urbs", "usus", "uter", "utor", "usus", "uxor", "vaco", "vado", "veni", "vere", "vero",
	"vidi", "vici", "vita", "vito", "vivo", "vixi", "voco", "volo", "sibi", "volo", "voro", "vovi",
};

static const char* fiveCharWords[] = {
	"abbas", "absum", "acies", "adamo", "adhuc", "adnuo", "adsum", "aduro", "aeger", "aegre", "eneus", "estas", "estus", "aetas", "aiunt", "alius",
	"aliud", "alter", "altus", "amita", "animi", "annus", "anser", "antea", "aptus", "arbor", "archa", "arceo", "arcus", "arguo", "ascit", "asper",
	"atrum", "atqui", "atrox", "audax", "audeo", "auris", "aurum", "autem", "autus", "aveho", "avoco", "barba", "bovis", "calco", "canis", "canto",
	"capio", "capto", "caput", "carbo", "careo", "carpo", "carus", "casso", "caste", "casus", "cauda", "causa", "caute", "caveo", "cavus", "cessi",
	"celer", "cerno", "certe", "certo", "cibus", "cinis", "civis", "clamo", "claro", "coepi", "colui", "comis", "nonis", "conor", "copia", "copie",
	"cornu", "credo", "creta", "cuius", "culpa", "culpo", "cunae", "cupio", "curia", "curis", "quris", "curso", "curto", "curvo", "damno", "debeo",
	"decet", "deleo", "demum", "denuo", "dicto", "diluo", "diruo", "datum", "doceo", "docui", "doleo", "dolor", "dolus", "domus", "donec", "donum",
	"dudum", "durus", "ducis", "edico", "essum", "educo", "eicio", "eligo", "epulo", "eques", "equus", "erepo", "erogo", "etiam", "evito", "evoco",
	"exigo", "eximo", "exoro", "exsto", "extra", "exuro", "facio", "fenum", "fatum", "faveo", "felix", "ferme", "nfere", "latum", "ferus", "feteo",
	"ficus", "fides", "filia", "fimus", "fines", "finis", "firmo", "fodio", "forma", "formo", "forte", "forum", "foveo", "fotum", "frons", "fruor",
	"fugio", "fulsi", "fundo", "funis", "furor", "fusus", "galea", "genus", "gesto", "gigno", "nonis", "agere", "gravo", "gusto", "habeo", "habui",
	"harum", "hodie", "honor", "ortus", "horum", "huius", "humus", "iaceo", "iacio", "ianua", "ictus", "eadem", "ignis", "ilico", "illae", "illas",
	"illud", "illic", "illis", "illos", "illuc", "illum", "imago", "imber", "ymber", "indux", "infit", "inflo", "infra", "innuo", "inops", "inruo",
	"irruo", "insto", "inter", "intro", "intus", "iocor", "iocus", "ipsum", "istud", "itero", "iubeo", "iussi", "iudex", "iugis", "iungo", "iunxi",
	"iuris", "iussu", "iuxta", "jugis", "labes", "labis", "labor", "solis", "lacer", "lacto", "lacus", "laedo", "lesio", "laeto", "letor", "laeve",
	"levus", "lambo", "lamia", "lamna", "latus", "laudo", "lebes", "legio", "lemma", "lenio", "lenis", "lente", "lento", "lepor", "lepos", "lepus",
	"letum", "levis", "legis", "liber", "libri", "liber", "licet", "limen", "locus", "longe", "ludio", "lusum", "ludum", "ludus", "lugeo", "lupus",
	"lucis", "macer", "macto", "mador", "maero", "meror", "magis", "opere", "maior", "magus", "peius", "malus", "malum", "peior", "mando", "maneo",
	"manus", "maris", "mater", "memor", "mensa", "mereo", "metim", "metuo", "metus", "milia", "mille", "minuo", "minui", "miror", "mirus", "miser",
	"missa", "mitis", "mitto", "modio", "modus", "moneo", "moris", "moveo", "motum", "mucro", "mugio", "multo", "mundo", "munio", "munus", "muris",
	"mutuo", "narro", "nasci", "natus", "natio", "nauta", "navis", "neque", "necne", "nefas", "nepos", "neque", "nidor", "nihil", "nimis", "niteo",
	"nitor", "nobis", "noceo", "nomen", "tenus", "nonus", "nosco", "novem", "novus", "nuper", "nutus", "obruo", "ocius", "odium", "omnis", "onero",
	"opera", "orbis", "orior", "oriri", "ortus", "noris", "otium", "paene", "npene", "palam", "palea", "palma", "pando", "parco", "pareo", "pario",
	"parum", "pasco", "pateo", "pater", "pauci", "pacis", "pecco", "pecto", "pecus", "peior", "pello", "pendo", "perdo", "pereo", "pergo", "pedis",
	"picea", "piger", "pigra", "pipio", "pirum", "pirus", "placo", "plaga", "plebs", "plene", "plico", "ploro", "pluit", "pluma", "album", "plura",
	"poema", "poena", "poeta", "pomum", "posui", "porro", "porta", "posco", "posse", "potum", "ferre", "preda", "preeo", "premo", "primo", "prior",
	"prius", "privo", "probo", "quasi", "prolu", "promo", "prope", "prout", "pruma", "pudeo", "pudor", "pugna", "pugno", "pulex", "pulmo", "pulpa",
	"pulso", "pungo", "punio", "purgo", "purus", "puteo", "puter", "putus", "quero", "queso", "nquam", "quare", "quasi", "quies", "etiam", "rapio",
	"rapui", "ratum", "rarus", "recro", "reddo", "redeo", "renuo", "repsi", "rideo", "risum", "rigor", "ritus", "rubor", "rumor", "rutum", "ruris",
	"saepe", "saeta", "salis", "salus", "sanus", "satio", "satis", "scivi", "secus", "atque", "secus", "sedeo", "semel", "senex", "senis", "sequi",
	"serio", "sermo", "servo", "sicut", "sidus", "sileo", "silva", "simul", "atque", "simul", "sitio", "sitis", "socer", "solis", "soleo", "solio",
	"solum", "solus", "solvo", "sopor", "spero", "nitis", "stips", "sulum", "summa", "supra", "surgo", "tabgo", "taceo", "talio", "talis", "talus",
	"tamen", "tenax", "tendo", "teneo", "tener", "tenus", "teres", "terga", "tergo", "tersi", "trivi", "terra", "theca", "thema", "timeo", "timor",
	"tollo", "totus", "trado", "traho", "traxi", "trans", "tremo", "trium", "turba", "turbo", "nonis", "turbo", "turpe", "tutis", "ullus", "ultio",
	"ultra", "umbra", "urbis", "uredo", "usque", "utrum", "valde", "valeo", "valui", "velox", "velum", "velut", "venia", "venio", "vergo", "verto",
	"verus", "vesco", "vetus", "video", "visum", "viduo", "vigor", "vilis", "villa", "vinco", "vinum", "virga", "virgo", "vires", "vobis", "velle",
	"volup", "volva", "vulva", "vomer", "vorax", "votum", "voveo", "votum", "vocis", "vulgo",
};

static const char* sixCharWords[] = {
	"abduco", "absens", "absque", "abutor", "accedo", "accuso", "acidus", "adduco", "adepto", "adfero", "affero", "adfero", "affero", "adicio", "adiuvo",
	"adopto", "adsumo", "advoco", "aegrus", "aeneus", "aequus", "nequus", "naeris", "aestas", "aestus", "aggero", "aliqua", "aliqui", "aliquo", "altera",
	"alveus", "ambulo", "amicus", "amitto", "amoveo", "amplus", "animus", "aperio", "aperte", "appono", "aranea", "nartis", "narcis", "astrum", "atavus",
	"attero", "auctor", "auctus", "aufero", "aureus", "avarus", "averto", "balbus", "bardus", "basium", "beatus", "bellum", "bellus", "bestia", "melior",
	"brevis", "caecus", "caelum", "calcar", "carcer", "caries", "catena", "cattus", "cautim", "cautum", "cessum", "centum", "certus", "cervus", "cetera",
	"ceteri", "cicuta", "clamor", "clarus", "claudo", "cogito", "ncoegi", "cohero", "cohesi", "cohors", "collum", "cultum", "coloro", "comedo", "comedi",
	"commeo", "compes", "consto", "consuo", "consui", "contra", "copiae", "corona", "corpus", "corruo", "creber", "crebro", "cresco", "cribro", "crinis",
	"crucio", "crucis", "cupido", "currus", "cursim", "cursor", "cursus", "curtus", "curvus", "custos", "decens", "decoro", "dedico", "deduco", "defero",
	"defigo", "defleo", "defluo", "degero", "deinde", "delego", "deludo", "demens", "denego", "depono", "deputo", "desino", "desiit", "desino", "desolo",
	"detego", "devito", "devoco", "dexter", "didico", "dididi", "didtum", "dignus", "dilato", "diligo", "doctum", "doctus", "dolens", "dolose", "domina",
	"domino", "domito", "dormio", "dubito", "dubium", "dulcis", "dusiol", "petram", "editio", "edoceo", "effero", "extuli", "elatum", "egenus", "elatus",
	"eluvio", "emanio", "emendo", "emerio", "emineo", "eminor", "eminus", "emiror", "emptum", "emoveo", "emptio", "epulor", "eripio", "erudio", "esurio",
	"etenim", "evenio", "everto", "exequo", "excedo", "excito", "excolo", "excuso", "exesto", "exibeo", "exilis", "exinde", "exitus", "expuli", "expers",
	"expeto", "expleo", "expono", "exulto", "fabula", "facile", "facina", "factum", "faenum", "falsus", "fateor", "fatigo", "fautor", "femina", "ferrum",
	"fidens", "filius", "finium", "nfieri", "firmus", "flamma", "flatus", "flumen", "nforem", "fortis", "forsit", "forsan", "fortis", "frango", "fracta",
	"frater", "frendo", "frigus", "nfrugi", "fulcio", "fulgeo", "fultus", "fungor", "nfungi", "furtim", "furtum", "gestum", "genuit", "gloria", "grando",
	"gratia", "gratus", "gravis", "helcim", "heniis", "hesito", "hortor", "hortus", "hospes", "hostes", "hostis", "hunnam", "igitur", "ignoro", "illata",
	"illiam", "illius", "illudo", "illusi", "imitor", "impedo", "impuli", "impero", "impleo", "imputo", "inanis", "incido", "incito", "incola", "incubo",
	"indico", "indidi", "induco", "infamo", "inferi", "infero", "infeci", "infigo", "infidi", "influo", "influi", "infodi", "infula", "ingens", "ingero",
	"inicio", "inieci", "inquam", "inquis", "inquit", "insons", "instar", "insula", "invado", "inviso", "invito", "iratus", "irrito", "itaque", "iterum",
	"iussum", "iudico", "iustus", "labium", "labiae", "lapsus", "laboro", "labrum", "lactis", "lacero", "lacuna", "laesio", "laetor", "laevus", "lamnia",
	"lapsus", "lector", "lectus", "legens", "lentis", "lentus", "leodie", "lepide", "libera", "libere", "libero", "libido", "lingua", "litigo", "longus",
	"loquax", "loquor", "lucror", "lucrum", "luctus", "ludius", "macero", "macies", "macula", "maculo", "madide", "maeror", "magnus", "malens", "mallui",
	"matris", "matera", "maxime", "medius", "mellis", "melior", "memini", "mentis", "mensis", "merces", "milies", "minime", "misceo", "miscui", "mixtum",
	"misere", "mitigo", "missum", "modica", "molior", "mollio", "mollis", "montis", "morbus", "morior", "mortis", "morsus", "mulier", "multum", "multus",
	"mundus", "munero", "mutuus", "nascor", "natura", "navigo", "quidem", "necdum", "nequam", "nequeo", "nescio", "nimium", "niveus", "nocens", "nnolle",
	"nnolui", "nomine", "nondum", "noster", "nostra", "noster", "nostri", "noster", "nostri", "noctis", "nullus", "quidem", "nuntio", "nutrio", "obduro",
	"obicio", "obieci", "obviam", "obvius", "occido", "occidi", "occupo", "oculus", "offero", "omitto", "omnino", "operor", "opinio", "oppono", "operis",
	"oratio", "orator", "ordine", "ostium", "pactum", "pactus", "parens", "paries", "partis", "partim", "minime", "parvus", "passer", "passim", "patris",
	"patior", "patria", "pauper", "pectus", "pulsum", "pendeo", "pepulo", "perimo", "peremi", "peruro", "pessum", "pestis", "pevela", "phasma", "pictor",
	"pigrum", "pignus", "piscis", "placeo", "placet", "plango", "planxi", "platea", "plebis", "plecto", "plenus", "pluvit", "plures", "pluris", "pluvia",
	"pocius", "potius", "pollen", "polleo", "pollex", "pontis", "possum", "postea", "potens", "potior", "potius", "dulcis", "prebeo", "precox", "praeda",
	"praeeo", "praemo", "presto", "presul", "presum", "precor", "pressi", "prenda", "precis", "primum", "privus", "procer", "procul", "profor", "frofui",
	"proles", "prolix", "proluo", "promus", "prosum", "puchre", "puella", "pugnax", "pugnus", "pullus", "pulsus", "pulvis", "pupugi", "puppis", "pupula",
	"puteus", "quaero", "quaeso", "qualis", "plures", "quando", "quanti", "quanto", "quarum", "quasso", "quater", "queror", "quibus", "quidam", "quedam",
	"quidam", "quidem", "quinam", "quenam", "quippe", "quoque", "quorum", "quovis", "recedo", "recepi", "recito", "recolo", "rectum", "rectus", "recuso",
	"redigo", "redono", "reduco", "refero", "regina", "regius", "regnum", "rectum", "regula", "relaxo", "relego", "relegi", "relevo", "repens", "repere",
	"repeto", "repleo", "reptum", "repono", "resumo", "revoco", "rhetor", "rursus", "sepius", "equina", "saevio", "salsus", "saltem", "saluto", "salveo",
	"salvus", "satago", "satura", "saturo", "scaber", "scelus", "schola", "scindo", "scitum", "scisco", "scribo", "habere", "secedo", "sessum", "semper",
	"sensus", "sentio", "septem", "sequax", "sequor", "serius", "servio", "servus", "siccus", "signum", "silens", "siligo", "socius", "solium", "somnio",
	"somnus", "sordeo", "sordes", "spargo", "sparsi", "specto", "specus", "sperno", "sprevi", "spolio", "sponte", "statim", "statua", "statuo", "stella",
	"stipes", "nsteti", "strues", "studio", "suadeo", "subito", "supero", "summus", "sursum", "tetigi", "tactum", "tabula", "tactus", "tamdiu", "tandem",
	"tantum", "tantus", "tardus", "tectum", "tempus", "tenera", "tenuis", "tergeo", "tersum", "tergum", "tergus", "termes", "tritum", "terreo", "terror",
	"tersus", "testis", "textor", "textus", "thesis", "thorax", "thymum", "tolero", "tondeo", "tonsum", "tonsor", "toties", "tracto", "tribuo", "turpis",
	"umerus", "umquam", "usitas", "ustilo", "ustulo", "utrius", "utilis", "utique", "utpote", "vacuus", "valens", "vallum", "varius", "ventum", "ventus",
	"nveris", "verbum", "vereor", "versus", "vesica", "vesper", "vester", "vestra", "vestio", "vestis", "vestri", "victor", "victus", "videor", "vigilo",
	"victum", "virtus", "viscus", "vitium", "victum", "dubium", "volens", "vomica", "vomito", "vorago", "vulgus", "vulnus", "vulpes", "volpes", "vultur",
	"voltur", "vultus"
};

static const char* sevenCharWords[] = {
	"abbatis", "abbatia", "abscido", "accendo", "accipio", "acerbus", "acervus", "acquiro", "adaugeo", "adeptio", "adficio", "affligo", "adhaero", "admiror",
	"admitto", "admoneo", "admoveo", "adsidue", "assidue", "adultus", "advenio", "adverto", "egresco", "equitas", "estivus", "eternus", "agnitio", "agnosco",
	"alienus", "alioqui", "aliquid", "aliquis", "aliquot", "allatus", "alterum", "ambitus", "amissio", "amissus", "ancilla", "angelus", "angulus", "appareo",
	"appello", "approbo", "arbitro", "arcesso", "ascisco", "aspicio", "asporto", "attollo", "audacia", "auditor", "auxatia", "baiulus", "benigne", "optimus",
	"calamus", "callide", "campana", "canonus", "capitis", "caritas", "caterva", "cautela", "censura", "cernuus", "ceterum", "ceterus", "carisma", "cineris",
	"civilis", "civitas", "claudeo", "clausus", "claudus", "coerceo", "cohaero", "cohesum", "cohibeo", "colligo", "colloco", "combibo", "comburo", "comesum",
	"comitis", "cometes", "comiter", "comitto", "commodo", "comparo", "competo", "compleo", "compono", "comptus", "conatus", "concedo", "concero", "concido",
	"concito", "condato", "condico", "conduco", "confero", "confido", "confugo", "conicio", "conitor", "coniuro", "consido", "consulo", "consumo", "contego",
	"contigo", "contigi", "convoco", "copiose", "corrigo", "cotidie", "crapula", "creator", "creptio", "cribrum", "ncruris", "cubitum", "cultura", "cunctor",
	"cunctus", "curatio", "curator", "cursito", "dapifer", "decerno", "decerto", "decimus", "decipio", "decorus", "decumbo", "dedecor", "dedecus", "defaeco",
	"defendo", "deficio", "defungo", "degusto", "delecto", "demergo", "demitto", "demoror", "denique", "deorsum", "depereo", "deporto", "deprimo", "depromo",
	"depulso", "derideo", "deripio", "desipio", "despero", "detineo", "devenio", "devotio", "devoveo", "dextera", "dictata", "dictito", "diffama", "differo",
	"digredi", "dilabor", "dimitto", "diripio", "discedo", "dispono", "disputo", "dissero", "distulo", "diutius", "divinus", "divitie", "dolosus", "dominus",
	"dulcedo", "dummodo", "eatenus", "ebullio", "econtra", "effervo", "efficio", "effrego", "effugio", "effundo", "eiectum", "electus", "eluvies", "emercor",
	"enumero", "equitis", "equidem", "eventus", "exaequo", "excipio", "exclamo", "excludo", "exerceo", "exertus", "exheres", "exhibeo", "eximius", "exitium",
	"exorior", "exorsus", "expedio", "expello", "explevi", "explico", "exposui", "expugno", "exequor", "exertus", "extollo", "exturbo", "exustio", "facilis",
	"familia", "famulus", "fefello", "felicis", "feritas", "festino", "fidelis", "fiducia", "nfactus", "nfalcis", "fluctus", "formica", "fortuna", "fructus",
	"frustra", "gaudium", "gladius", "glorior", "grassor", "habitum", "habitus", "harniis", "haesito", "hilaris", "hominis", "hordeum", "nordeum", "hostium",
	"humanus", "humilis", "iaculum", "idcirco", "idoneus", "ignarus", "ignavus", "ignosco", "ignotus", "illarum", "illorum", "illusum", "imbrium", "immineo",
	"immotus", "immunda", "impedio", "impello", "impendo", "impensa", "impetro", "impetus", "importo", "incipio", "inclino", "includo", "increpo", "incurro",
	"indigeo", "ineptio", "reddere", "infelix", "infenso", "inferne", "inferus", "infeste", "infesto", "inficio", "infidus", "infindo", "infirme", "infirmo",
	"inflexi", "infligo", "inflixi", "infodio", "infimus", "ingemuo", "iniquus", "initium", "iniuria", "inrideo", "inritus", "irritus", "insania", "insideo",
	"insinuo", "insisto", "instigo", "instruo", "insurgo", "insurgi", "integer", "intendo", "intereo", "intueor", "inultus", "invenio", "invicem", "invideo",
	"invidia", "invisus", "invitus", "ipsemet", "irascor", "irritum", "irritus", "iunctum", "iurandi", "jaculum", "jugiter", "juvenis", "laboris", "labores",
	"lacerta", "lacesso", "lacrima", "lacrimo", "lactans", "lacteus", "lactuca", "lacunar", "laetans", "letatio", "letitia", "laganum", "lamenta", "lammina",
	"laqueum", "laqueus", "largior", "lasesco", "lectica", "legatus", "lemures", "lenitas", "lepidus", "letalis", "letanie", "letifer", "levamen", "levatio",
	"levitas", "leviter", "libatio", "nlimina", "linteum", "littera", "lateque", "locutus", "lucerna", "luxuria", "madesco", "madidus", "maximus", "maiores",
	"pessime", "mancipo", "mancepo", "mancipo", "maritus", "nmairia", "materia", "medicus", "meditor", "memoria", "nmereor", "militis", "millies", "minimus",
	"minutum", "misereo", "mitesco", "modicus", "moleste", "monitio", "monstro", "nmorium", "mortuus", "munitio", "muneris", "mussito", "mutatio", "natalis",
	"necesse", "nepotis", "nihilum", "nimirum", "nitesco", "nonnisi", "nostrum", "novitas", "numerus", "numquam", "nunquam", "nuntius", "nusquam", "obliquo",
	"oblivio", "obtineo", "occasio", "occasum", "occulto", "occurro", "oportet", "opposui", "opprimo", "oppugno", "optimus", "ornatus", "ostendo", "paganus",
	"pallium", "paratus", "parilis", "pariter", "minimus", "patiens", "patruus", "pecunia", "pepulli", "penitus", "peracto", "peragro", "percepi", "perduco",
	"perfero", "perfeci", "peritus", "perseco", "perussi", "pervidi", "petitus", "placide", "plactum", "plector", "plumbum", "plurimi", "polenta", "positum",
	"poposco", "populus", "positus", "posteri", "postulo", "praebeo", "precedo", "praecox", "predico", "prefero", "prefeci", "prefoco", "premium", "prepono",
	"praesto", "praesul", "praesum", "presumo", "praeter", "pressum", "pretium", "priores", "priscus", "procedo", "profero", "profari", "proicio", "proinde",
	"prolato", "prolixi", "prompsi", "promovi", "prompte", "promptu", "propero", "propono", "proprie", "propter", "prorsus", "prosper", "proveho", "prudens",
	"pudicus", "pulcher", "pulchra", "pullulo", "pumilio", "punctum", "punitor", "pupilla", "purpura", "putator", "putesco", "pyropus", "pyxidis", "quadrum",
	"questio", "questus", "quamdiu", "quamvis", "quantum", "quantus", "quartus", "quercus", "quereia", "quernus", "quaedam", "quietis", "quaenam", "quodnam",
	"quisnam", "quisque", "quomodo", "quondam", "quoniam", "recipio", "recondo", "redundo", "reformo", "remando", "remaneo", "removeo", "rependo", "repente",
	"reperio", "repugno", "requiro", "publica", "resisto", "retineo", "retraho", "retraxi", "revenio", "reverto", "reverti", "revolvo", "rostrum", "saepius",
	"salutor", "sanctus", "egidius", "sanctus", "sanitas", "sapiens", "sarcina", "scabies", "scaldus", "scamnum", "sciphus", "scelero", "secerno", "secrevi",
	"securus", "seditio", "seorsum", "sepelio", "seputus", "secutus", "sibimet", "silenti", "similis", "simplex", "singuli", "sollers", "solutio", "somnium",
	"sonitus", "sparsum", "spretum", "spolium", "nstipis", "nstatum", "studium", "stultus", "subitus", "sublime", "subseco", "succedo", "suffoco", "suggero",
	"futurus", "sumptus", "superna", "superne", "superus", "supplex", "suppono", "surrexi", "suscito", "tabella", "tabesco", "taedium", "ntedium", "tametsi",
	"tamquam", "tanquam", "tempero", "templum", "tenerum", "tepesco", "tepidus", "terebro", "termino", "territo", "tertius", "textrix", "thermae", "thymbra",
	"timidus", "titulus", "sustuli", "totondi", "torqueo", "torrens", "totidem", "ntoties", "tradidi", "tractum", "transeo", "trellum", "trepide", "triduum",
	"tristis", "fontium", "trucido", "tumulus", "tungris", "tutamen", "humerus", "undique", "urbanus", "uterque", "utroque", "validus", "vapulus", "ventito",
	"verbera", "veritas", "nvescor", "vespera", "vestrum", "vestivi", "vestrum", "vicinus", "viduata", "vilicus", "vilitas", "vindico", "vinitor", "viridis",
	"vulnero", "xiphias",
};

static const char* eightCharWords[] = {
	"absorbeo", "abstergo", "abundans", "acceptus", "ademptio", "adfectus", "affectus", "adflicto", "adimpleo", "adsuesco", "assuesco", "adulatio", "adversus",
	"aegresco", "egretudo", "aequitas", "aestivus", "aeternus", "aldenard", "alioquin", "ambianis", "amicitia", "amiculum", "amplexus", "angustus", "antepono",
	"antiquus", "arbitror", "arbustum", "arbustus", "naccerso", "argentum", "armarium", "audacter", "audentia", "auxilium", "avaritia", "avesniis", "bellicus",
	"blandior", "blesense", "brevitas", "breviter", "calculus", "callidus", "candidus", "capillus", "carbonis", "cariosus", "carnutum", "carnotum", "celebrer",
	"celebrus", "charisma", "cilicium", "clibanus", "cognatus", "cognomen", "cognosco", "ncoactum", "cohortor", "comminor", "comminuo", "comminus", "commodum",
	"commoneo", "commoveo", "communis", "compater", "compello", "comperio", "comperte", "comprobo", "concepta", "concipio", "concisus", "conculco", "concutio",
	"conforto", "congrego", "congruus", "coniecto", "conscius", "conservo", "consilio", "consisto", "consitor", "constans", "construo", "consueta", "consulto",
	"consummo", "consutum", "consurgo", "contages", "contagio", "contemno", "contendo", "contente", "contineo", "contingo", "continuo", "contrado", "contraho",
	"conturbo", "converto", "corbeiam", "corporis", "corripio", "corrumpo", "coruscus", "creatura", "crinitus", "crudelis", "cruentus", "cunabula", "cuppedia",
	"curiosus", "custodia", "custodie", "damnatio", "debilito", "decretum", "defessus", "degenero", "delibero", "delicate", "deliciae", "delinquo", "demulceo",
	"denuncio", "denuntio", "depopulo", "depredor", "deprecor", "desidero", "despecto", "despicio", "desposco", "destituo", "detectum", "diabolus", "dictator",
	"digestor", "dignitas", "dignosco", "diligens", "diluculo", "dimidium", "directus", "discrepo", "diutinus", "diversus", "divitiae", "doctrina", "dumtaxat",
	"ecclesia", "ecquando", "effectus", "effringo", "egredior", "eloquens", "epistula", "erubesco", "excessum", "excrucio", "exemplar", "exemplum", "exheredo",
	"exhilaro", "exitosus", "exordium", "expulsum", "experior", "experiri", "expetens", "expiscor", "expletum", "expletio", "expletus", "exsequor", "exsertus",
	"exsilium", "exspecto", "externus", "extremus", "nexussum", "facultas", "facundia", "fenestra", "feretrum", "festinus", "forensis", "forsitan", "fortasse",
	"fortiter", "nfunctus", "gandavum", "gratulor", "gravatus", "gravitas", "graviter", "gregatim", "hactenus", "hoienses", "immerito", "immundus", "impedito",
	"impulsum", "impendeo", "imperium", "imprimis", "improbus", "impudens", "incassum", "inceptor", "inceptum", "incertus", "inclutus", "inclitus", "indignus",
	"inductum", "indutiae", "infantia", "infector", "infectum", "infectus", "infensus", "infestus", "infectum", "infissum", "infirmus", "infitias", "infitior",
	"inflammo", "inflatio", "inflatus", "inflecto", "infletus", "inflexio", "inflexus", "influxum", "infossum", "informis", "inferius", "ingenium", "ingratus",
	"iniectum", "inimicus", "iniustus", "ninnotui", "inolesco", "insciens", "inscribo", "insequor", "inservio", "insidiae", "insolita", "insontis", "instituo",
	"intentio", "intentus", "interdum", "inventor", "invetero", "invictus", "itineris", "iucundus", "iudicium", "iumentum", "iurandum", "judicium", "jumentum",
	"juventus", "labefeci", "labellum", "labiosus", "labrusca", "lacertus", "lactatio", "laetatio", "letifico", "laetitia", "lamentor", "lascivio", "laudator",
	"laudunum", "legentis", "lemiscus", "lenitudo", "lentesco", "lentulus", "libellus", "libenter", "nliberum", "libertas", "liquidus", "litterae", "loquacis",
	"lubricus", "lucrosus", "macresco", "magister", "maiestas", "pessimus", "mandatum", "manentia", "mellitus", "membrana", "mendosus", "mercedis", "meretrix",
	"mestitia", "ministro", "modestus", "molestia", "molestus", "monachus", "monstrum", "moratlis", "plurimum", "munerior", "negotium", "nequitia", "notarius",
	"obdormio", "obiectum", "obtestor", "occursus", "offensio", "officina", "officium", "oppressi", "terrarum", "nordinem", "paciscor", "parentis", "parietis",
	"paternus", "patronus", "paulatim", "peccatus", "pectoris", "percipio", "percutio", "perficio", "perfruor", "perfusus", "permitto", "permissi", "permoveo",
	"perperam", "perpetro", "persequi", "persisto", "persolvo", "personam", "persuasi", "pertinax", "pertineo", "pertingo", "pertraho", "perturbo", "perustum",
	"pervenio", "perverto", "perverti", "pervideo", "pervisum", "pessimus", "pestifer", "pharetra", "npiperis", "piscator", "placidus", "placitum", "plorator",
	"ploratus", "plumbeus", "plurimus", "poematis", "posterus", "postpono", "postquam", "potestas", "praecedo", "precepio", "praecido", "preconor", "praedico",
	"praefero", "preficio", "prefinio", "praefoco", "pregravo", "praemium", "praepono", "prestans", "praesumo", "preterea", "pretereo", "pravitas", "precipio",
	"precipue", "prehendo", "pretereo", "primitus", "primoris", "princeps", "privatus", "privigna", "probitas", "procella", "proditor", "proelium", "profecto",
	"proficio", "profatus", "profugus", "profundo", "profusum", "progener", "progigno", "progenui", "prohibeo", "prolabor", "prolatio", "prolecto", "prolicio",
	"prolixus", "prolutum", "prolusio", "promereo", "promineo", "promisce", "promitto", "promptum", "promoveo", "promotum", "promptus", "pronepos", "propello",
	"proprius", "protinus", "protraho", "provideo", "provisor", "proximus", "publicus", "pudendus", "puerilis", "pulchrum", "pulpitum", "pumilius", "puniceus",
	"pupillus", "purgatio", "pusillus", "quesitio", "quaestio", "quaestus", "qualitas", "qualiter", "quamquam", "quatenus", "quatinus", "quatenus", "quatinus",
	"quattuor", "querella", "queritor", "querulus", "quicquid", "quilibet", "nquidnam", "quisquam", "quisquis", "quotiens", "receptum", "recordor", "recupero",
	"redarguo", "redactum", "relectum", "relictus", "relinquo", "reliquum", "reluctor", "renuntio", "repletus", "requievi", "respicio", "respondi", "restituo",
	"resumpsi", "retribuo", "revertor", "reversus", "rotundus", "rusticus", "sabbatum", "sacculus", "rodoenus", "santiago", "scaphium", "sceleris", "scientia",
	"scilicet", "nscripsi", "scriptum", "scrinium", "scriptor", "astringo", "secretum", "secundum", "secundus", "secuutus", "seductor", "senectus", "servitus",
	"singulus", "siquidem", "solitudo", "sordesco", "sortitus", "speculum", "spiculum", "spiritus", "stabilis", "strenuus", "studiose", "suasoria", "subiungo",
	"subnecto", "subvenio", "succendo", "succurro", "sufficio", "summisse", "summitto", "supellex", "superbia", "superbus", "supernus", "supersum", "superior",
	"supremus", "surculus", "suscipio", "suspendo", "suspendi", "sustineo", "synagoga", "tabellae", "tabernus", "tamisium", "temporis", "terminus", "textilis",
	"theatrum", "thematis", "sublatum", "ntotiens", "traditum", "tredecim", "treverim", "triduana", "triginta", "tripudio", "tubineus", "tumultus", "turbatio",
	"turbatus", "tyrannus", "uberrime", "ulciscor", "ulterius", "ultionis", "universe", "universi", "utilitas", "utrimque", "valetudo", "varietas", "vehemens",
	"ventosus", "venustas", "vespillo", "vestitum", "victoria", "vinculum", "vitiosus", "voluntas", "voluptas", "vulgaris",
};

static const char* nineCharWords[] = {
	"nabsentis", "accommodo", "accusator", "acerbitas", "adipiscor", "admiratio", "admonitio", "adstringo", "edificium", "aegretudo", "egrotatio", "aggredior",
	"aliquando", "aliquanta", "aliquanto", "amaritudo", "amplitudo", "apostolus", "apparatus", "appositus", "articulus", "asperitas", "asvesniis", "atrebatum",
	"atrocitas", "blanditia", "brachants", "caelestis", "calamitas", "cameracum", "canonicus", "capitulus", "celeritas", "celeriter", "cenaculum", "ciminatio",
	"ciminosus", "claustrum", "clementia", "coloratus", "cometissa", "comitissa", "comitatus", "commemoro", "commisceo", "commissum", "compatior", "ncompedis",
	"concilium", "confestim", "confiteor", "confessus", "conqueror", "conscendo", "conscindo", "considero", "consilium", "conspergo", "conspicio", "constituo",
	"constrixi", "construxi", "constupro", "consuasor", "consuesco", "consultum", "contactus", "contagium", "contamino", "contentus", "continuus", "contristo",
	"conventus", "corroboro", "courtacum", "crastinus", "crustulum", "cubiculum", "cuiusmodi", "cultellus", "cunctatio", "cunctator", "cupiditas", "cupressus",
	"curtracus", "custodiae", "ndecenter", "demonstro", "depopulor", "depraedor", "depressus", "determino", "didicerat", "differtus", "digredior", "digressus",
	"digressio", "digressus", "dilgenter", "dirunitas", "discidium", "dissimulo", "distinguo", "distribuo", "distringo", "diuturnus", "divinitus", "dominatus",
	"dulcidine", "dulcitudo", "elementum", "elemosina", "episcopus", "equitatus", "excusatio", "exercitus", "exitialis", "nexpertus", "expilatio", "expositum",
	"expositus", "expostulo", "exstinguo", "extorqueo", "famulatus", "feliciter", "fidelitas", "finitimus", "finitumus", "fortitudo", "fortunate", "frequento",
	"frumentum", "fugitivus", "glacialis", "hasnonium", "hereditas", "hodiernus", "horrendus", "humanitas", "hypocrita", "iaculator", "identidem", "illacrimo",
	"illaturos", "immanitas", "immodicus", "impendium", "imperator", "improviso", "impunitus", "mentionem", "increpare", "indagatio", "indebitus", "indomitus",
	"industria", "infidelis", "infinitas", "infinitio", "infinitus", "inflatius", "inflectum", "inflictum", "innotesco", "insolitus", "instanter", "intellego",
	"intellexi", "intercepi", "interdico", "interfeci", "introduco", "intumesco", "invalesco", "investigo", "labefacio", "labefacto", "laboriose", "labruscum",
	"laceratio", "heliandum", "letabilis", "laetifico", "letificus", "lenocinor", "lentitudo", "lesciense", "letaliter", "lethargus", "leviculus", "liberalis",
	"liberatio", "locupleto", "loricatus", "maculosus", "magnopere", "mansuetus", "matertera", "mediocris", "meditatus", "meminisse", "memoratus", "militaris",
	"nmisereor", "montensem", "mortifera", "namucense", "navigatio", "nequaquam", "nequities", "nominatim", "nonnullus", "nutrimens", "obligatus", "oblittero",
	"obsequium", "omnigenus", "oppositum", "opportune", "oppressum", "optimates", "ordinatio", "patefacio", "patientia", "paupertas", "perceptum", "percontor",
	"perculsus", "percussum", "perdignus", "perfectus", "perfectum", "periculum", "peremptum", "periurium", "perlustro", "permissum", "perpetuus", "perscitus",
	"perscribo", "persequor", "persevero", "persuadeo", "persuasum", "perterreo", "perturpis", "perversum", "pestifere", "phasmatis", "plaustrum", "plerumque",
	"plerusque", "plusculus", "pluvialis", "polliceor", "possessio", "npostremo", "posthabeo", "precelsus", "praecepio", "preceptum", "praecipio", "nprecipio",
	"precipuus", "preclarus", "praeconor", "praeficio", "prefectum", "praefinio", "praegravo", "praemitto", "presentia", "presencia", "presidium", "praestans",
	"praeterea", "praetereo", "praevenio", "nprevenio", "praevenio", "prevenire", "pristinus", "priusquam", "procinctu", "proficuus", "profiteor", "profundum",
	"profundus", "progenero", "progenies", "prognatus", "prolapsio", "proloquor", "proluvier", "promereor", "prominens", "promiscue", "promiscus", "promissio",
	"promissor", "promutuus", "proneptos", "pronuntio", "propinquo", "prosequor", "protestor", "nprotesto", "provectus", "proventus", "prudenter", "prudentia",
	"pulmentum", "quadratus", "quadrigae", "quaesitio", "quamobrem", "quantuvis", "quassatio", "quercetum", "quingenti", "radicitus", "redemptio", "redemptor",
	"relucesco", "rememdium", "remuneror", "repetitio", "requiesco", "requietum", "respondeo", "responsum", "retractum", "sapienter", "sapientia", "scelestus",
	"sententia", "severitas", "siclinium", "silentium", "simulatio", "singultim", "singultus", "sodalitas", "sollicito", "speciosus", "spoliatio", "richarius",
	"stabulaus", "subsequor", "successio", "summissus", "summopere", "superfluo", "suppellex", "supplanto", "surrectum", "suspensum", "tantillus", "taruennam",
	"temeritas", "tempestas", "temptatio", "theologus", "thesaurus", "traiectum", "transfero", "transtuli", "triduanus", "triumphus", "tutaminis", "universum",
	"universus", "valiturus", "velociter", "vendolius", "vestigium", "videlicet", "nvillicus", "viriliter", "vociferor", "volaticus", "volatilis", "volubilis",
	"vulariter", "vultuosus", "vulturius", "volturius",
};

static const char* tenCharWords[] = {
	"abundantia", "adulescens", "aedificium", "aegrotatio", "aliquantum", "aliquantus", "argumentum", "assentator", "attonbitus", "auctoritas", "naudaciter",
	"beneficium", "boloniense", "brocherota", "carnotense", "catervatim", "coadunatio", "coaegresco", "complectus", "compositio", "compositus", "concupisco",
	"coniuratio", "coniuratus", "constanter", "constringo", "consuetudo", "consulatio", "contabesco", "contemplor", "contemptim", "contemptio", "contemptus",
	"ncontectum", "correptius", "curiositas", "curriculum", "defetiscor", "delectatio", "deprecator", "derelinquo", "desidiosus", "desparatus", "despiciens",
	"destinatus", "difficilis", "diligentia", "discipulus", "disputatio", "dissimilis", "dissolutus", "districtus", "domesticus", "eloquentia", "exhorresco",
	"eximietate", "explicatus", "exquisitus", "nexstingui", "facillimus", "facunditas", "familiaris", "feculentia", "festinatio", "fortunatus", "frequentia",
	"frugalitas", "furibundus", "furtificus", "imitabilis", "immortalis", "imperiosus", "importunus", "improvidus", "impudenter", "praesentia", "inconsulte",
	"indignatio", "industrius", "infecundus", "infervesco", "infirmatio", "infirmitas", "infitialis", "informatio", "ingravesco", "insensatus", "insperatus",
	"instructus", "intercipio", "interficio", "iucunditas", "labefactum", "laboriosus", "lacertosus", "lacrimosus", "laetabilis", "laetificus", "lamentatio",
	"legatarius", "lenimentus", "lenocinium", "nmonastery", "levamentum", "levidensis", "luctisonus", "lutosensis", "mactabilis", "menapiorum", "meretricis",
	"meridianus", "munimentum", "nonnumquam", "obstinatus", "omnipotens", "oporotheca", "opportunus", "opprobrium", "patrocinor", "percunctor", "peregrinus",
	"periclitor", "persecutus", "perspicuus", "pertimesco", "pertinacia", "pertorqueo", "pervalidus", "phitonicum", "pictoratus", "plagiarius", "plorabilis",
	"pollicitus", "potissimum", "potissimus", "praecelsus", "praeceptum", "praecipuus", "praeclarus", "praenuntio", "nprenuntio", "prenuncius", "prepositus",
	"praesentia", "praesidium", "prestantia", "preteritus", "principium", "procurator", "progenitor", "progenitum", "progredior", "progressio", "progressus",
	"prohibitio", "promeritum", "promiscuus", "propinquus", "propositum", "prosecutus", "profuturus", "protractus", "provolvere", "pueriliter", "pugnacitas",
	"pugnaculum", "pulchellus", "quadrivium", "quadruplor", "questuosus", "quamtotius", "quantocius", "quantotius", "quantocius", "quantotius", "quapropter",
	"querimonia", "quodammodo", "recognosco", "reconcilio", "recordatio", "reprehendo", "nresumptum", "rhetoricus", "rudimentum", "sacrificum", "sacrilegus",
	"sanctifico", "sceleratus", "sepulchrum", "similitudo", "singularis", "sollicitus", "sophismata", "stabilitas", "substantia", "suffragium", "supplicium",
	"tantummodo", "templovium", "terminatio", "tornacense", "translatum", "transmitto", "tricesimus", "uticensium", "vehementer", "verecundia", "verumtamen",
	"vindicatum", "volutabrum", "vulgivagus", "vulticulus",
};

static const char* symbolChars = "1234567890;':\",./<>?[]\\{}|-=_+`~!@#$%^&*()";

Stage stages[] = {
	{
		-1.0f, false, false
	},
	{
		4.0f, false, false
	},
	{
		3.75f, false, false
	},
	{
		3.5f, true, false
	},
	{
		3.25f, true, true
	},
	{
		3.0f, true, true
	},
	{
		2.8f, true, true
	},
	{
		2.6f, true, true
	},
	{
		2.4f, true, true
	},
	{
		2.2f, true, true
	},
	{
		2.0f, true, true
	},
	{
		1.9f, true, true
	},
	{
		1.8f, true, true
	},
	{
		1.7f, true, true
	},
	{
		1.6f, true, true
	},
	{
		1.5f, true, true
	},
	{
		1.4f, true, true
	},
	{
		1.3f, true, true
	},
	{
		1.2f, true, true
	},
	{
		1.1f, true, true
	},
	{
		1.0f, true, true
	},
};

//*****************************************************************************************************

typedef struct {
	float timePassed;
	float time;
	float start;
	float target;
	EaseFunc ease;
} FloatLerpData;

typedef struct {
	float timePassed;
	float time;
	Vector2 start;
	Vector2 target;
	EaseFunc ease;
} Vec2LerpData;

typedef struct {
	float timePassed;
	float time;
	Color start;
	Color target;
	EaseFunc ease;
} ColorLerpData;

typedef struct {
	float timeLeft;
} LifeTimeData;

typedef struct {
	Vector2 vel;
	Vector2 gravScale;
} KinematicData;

ComponentID val0LerpCompID = INVALID_ENTITY_ID;
ComponentID colorLerpCompID = INVALID_ENTITY_ID;
ComponentID lifeTimeCompID = INVALID_ENTITY_ID;
ComponentID posLerpCompID = INVALID_ENTITY_ID;
ComponentID rotLerpCompID = INVALID_ENTITY_ID;
ComponentID kinematicCompID = INVALID_ENTITY_ID;
ComponentID scaleLerpCompID = INVALID_ENTITY_ID;

static void registerComponents( void )
{
	val0LerpCompID = ecps_AddComponentType( &gameECPS, "VAL0_LERP", sizeof( FloatLerpData ), ALIGN_OF( FloatLerpData ), NULL, NULL );
	colorLerpCompID = ecps_AddComponentType( &gameECPS, "CLR_LERP", sizeof( ColorLerpData ), ALIGN_OF( ColorLerpData ), NULL, NULL );
	lifeTimeCompID = ecps_AddComponentType( &gameECPS, "LIFETIME", sizeof( LifeTimeData ), ALIGN_OF( LifeTimeData ), NULL, NULL );
	posLerpCompID = ecps_AddComponentType( &gameECPS, "POS_LERP", sizeof( Vec2LerpData ), ALIGN_OF( Vec2LerpData ), NULL, NULL );
	rotLerpCompID = ecps_AddComponentType( &gameECPS, "ROT_LERP", sizeof( FloatLerpData ), ALIGN_OF( FloatLerpData ), NULL, NULL );
	kinematicCompID = ecps_AddComponentType( &gameECPS, "KIN", sizeof( KinematicData ), ALIGN_OF( KinematicData ), NULL, NULL );
	scaleLerpCompID = ecps_AddComponentType( &gameECPS, "SCL_LERP", sizeof( Vec2LerpData ), ALIGN_OF( Vec2LerpData ), NULL, NULL );
}




//*****************************************************************************************************

static float procDT;
static Process colorLerpProc;
static Process val0LerpProc;
static Process lifeTimeProc;
static Process posLerpProc;
static Process rotLerpProc;
static Process simplePhysicsProc;
static Process scaleLerpProc;

static void processColorLerp( ECPS* ecps, const Entity* entity )
{
	GCColorData* clr = NULL;
	ColorLerpData* lerpData = NULL;
	ecps_GetComponentFromEntity( entity, colorLerpCompID, &lerpData );
	ecps_GetComponentFromEntity( entity, gcClrCompID, &clr );

	lerpData->timePassed += procDT;

	EaseFunc ease = lerpData->ease;
	if( ease == NULL ) {
		ease = easeLinear;
	}
	float t = lerpData->timePassed / lerpData->time;
	t = MIN( 1.0f, t );

	clr_Lerp( &( lerpData->start ), &( lerpData->target ), t, &( clr->futureClr ) );

	if( lerpData->timePassed >= lerpData->time ) {
		ecps_RemoveComponentFromEntityMidProcess( ecps, entity, colorLerpCompID );
	}
}

static void processVal0Lerp( ECPS* ecps, const Entity* entity )
{
	GCFloatVal0Data* val0 = NULL;
	FloatLerpData* lerpData = NULL;
	ecps_GetComponentFromEntity( entity, val0LerpCompID, &lerpData );
	ecps_GetComponentFromEntity( entity, gcFloatVal0CompID, &val0 );

	lerpData->timePassed += procDT;

	EaseFunc ease = lerpData->ease;
	if( ease == NULL ) {
		ease = easeLinear;
	}
	float t = lerpData->timePassed / lerpData->time;
	t = MIN( 1.0f, t );
	val0->futureValue = lerp( lerpData->start, lerpData->target, ease( t ) );

	if( lerpData->timePassed >= lerpData->time ) {
		ecps_RemoveComponentFromEntityMidProcess( ecps, entity, val0LerpCompID );
	}
}

static void processPosLerp( ECPS* ecps, const Entity* entity )
{
	GCPosData* pos = NULL;
	Vec2LerpData* lerpData = NULL;
	ecps_GetComponentFromEntity( entity, posLerpCompID, &lerpData );
	ecps_GetComponentFromEntity( entity, gcPosCompID, &pos );

	lerpData->timePassed += procDT;

	EaseFunc ease = lerpData->ease;
	if( ease == NULL ) {
		ease = easeLinear;
	}
	float t = lerpData->timePassed / lerpData->time;
	t = MIN( 1.0f, t );
	vec2_Lerp( ( &lerpData->start ), ( &lerpData->target ), ease( t ), ( &pos->futurePos ) );

	if( lerpData->timePassed >= lerpData->time ) {
		ecps_RemoveComponentFromEntityMidProcess( ecps, entity, posLerpCompID );
	}
}

static void processLifeTime( ECPS* ecps, const Entity* entity )
{
	LifeTimeData* life = NULL;

	ecps_GetComponentFromEntity( entity, lifeTimeCompID, &life );

	life->timeLeft -= procDT;

	if( life->timeLeft <= 0.0f ) {
		ecps_DestroyEntity( ecps, entity );
	}
}

static Vector2 gravity = { 0.0f, 1500.0f };
static void processSimplePhysics( ECPS* ecps, const Entity* entity )
{
	GCPosData* posData = NULL;
	KinematicData* kineData = NULL;

	ecps_GetComponentFromEntity( entity, gcPosCompID, &posData );
	ecps_GetComponentFromEntity( entity, kinematicCompID, &kineData );

	Vector2 scaledGravity;
	vec2_HadamardProd( &gravity, &( kineData->gravScale ), &scaledGravity );

	// update position
	Vector2 basePos = posData->futurePos;

	vec2_AddScaled( &basePos, &( kineData->vel ), procDT, &basePos );
	vec2_AddScaled( &basePos, &scaledGravity, 0.5f * procDT * procDT, &basePos );
	posData->futurePos = basePos;

	// update velocity
	vec2_AddScaled( &( kineData->vel ), &scaledGravity, procDT, &( kineData->vel ) );
}

static void processScaleLerp( ECPS* ecps, const Entity* entity )
{
	GCScaleData* scale = NULL;
	Vec2LerpData* lerpData = NULL;
	ecps_GetComponentFromEntity( entity, scaleLerpCompID, &lerpData );
	ecps_GetComponentFromEntity( entity, gcScaleCompID, &scale );

	lerpData->timePassed += procDT;

	EaseFunc ease = lerpData->ease;
	if( ease == NULL ) {
		ease = easeLinear;
	}
	float t = lerpData->timePassed / lerpData->time;
	t = MIN( 1.0f, t );
	vec2_Lerp( ( &lerpData->start ), ( &lerpData->target ), ease( t ), ( &scale->futureScale ) );

	if( lerpData->timePassed >= lerpData->time ) {
		ecps_RemoveComponentFromEntityMidProcess( ecps, entity, scaleLerpCompID );
	}
}

static void registerProcesses( void )
{
	ecps_CreateProcess( &gameECPS, "VAL0_LERP", NULL, processVal0Lerp, NULL, &val0LerpProc, 2, val0LerpCompID, gcFloatVal0CompID );
	ecps_CreateProcess( &gameECPS, "CLR_LERP", NULL, processColorLerp, NULL, &colorLerpProc, 2, colorLerpCompID, gcClrCompID );
	ecps_CreateProcess( &gameECPS, "POS_LERP", NULL, processPosLerp, NULL, &posLerpProc, 2, posLerpCompID, gcPosCompID );
	ecps_CreateProcess( &gameECPS, "LIFE_TIME", NULL, processLifeTime, NULL, &lifeTimeProc, 1, lifeTimeCompID );
	ecps_CreateProcess( &gameECPS, "PHYSICS", NULL, processSimplePhysics, NULL, &simplePhysicsProc, 2, gcPosCompID, kinematicCompID );
	ecps_CreateProcess( &gameECPS, "SCL_LERP", NULL, processScaleLerp, NULL, &scaleLerpProc, 2, scaleLerpCompID, gcScaleCompID );
}

//*****************************************************************************************************

static void addColorLerp( EntityID id, float time, Color endColor, EaseFunc ease )
{
	ColorLerpData* currLerp = NULL;
	GCColorData* color = NULL;

	if( !ecps_GetComponentFromEntityByID( &gameECPS, id, gcClrCompID, &color ) ) {
		llog( LOG_WARN, "Attempting to add color lerp to entity with no color." );
		return;
	}

	if( ecps_GetComponentFromEntityByID( &gameECPS, id, colorLerpCompID, &currLerp ) ) {
		// modify existing one
		currLerp->time = time;
		currLerp->timePassed = 0.0f;
		currLerp->start = color->currClr;
		currLerp->target = endColor;
		currLerp->ease = ease;
	} else {
		// add new one
		ColorLerpData lerpData;
		lerpData.time = time;
		lerpData.timePassed = 0.0f;
		lerpData.start = color->currClr;
		lerpData.target = endColor;
		lerpData.ease = ease;

		ecps_AddComponentToEntityByID( &gameECPS, id, colorLerpCompID, &lerpData );
	}
}

static void addAlphaLerp( EntityID id, float time, float endAlpha, EaseFunc ease )
{
	GCColorData* color = NULL;

	if( !ecps_GetComponentFromEntityByID( &gameECPS, id, gcClrCompID, &color ) ) {
		llog( LOG_WARN, "Attempting to add alpha lerp to entity with no color." );
		return;
	}

	Color c = color->currClr;
	c.a = endAlpha;
	addColorLerp( id, time, c, ease );
}

static void addPosLerp( EntityID id, float time, Vector2 endPos, EaseFunc ease )
{
	Vec2LerpData* currLerp = NULL;
	GCPosData* pos = NULL;

	if( !ecps_GetComponentFromEntityByID( &gameECPS, id, gcPosCompID, &pos ) ) {
		llog( LOG_WARN, "Attempting to add position lerp to entity with no position." );
		return;
	}

	if( ecps_GetComponentFromEntityByID( &gameECPS, id, posLerpCompID, &currLerp ) ) {
		// modify existing one
		currLerp->time = time;
		currLerp->timePassed = 0.0f;
		currLerp->start = pos->currPos;
		currLerp->target = endPos;
		currLerp->ease = ease;
	} else {
		// add new one
		Vec2LerpData lerpData;
		lerpData.time = time;
		lerpData.timePassed = 0.0f;
		lerpData.start = pos->currPos;
		lerpData.target = endPos;
		lerpData.ease = ease;

		ecps_AddComponentToEntityByID( &gameECPS, id, posLerpCompID, &lerpData );
	}
}

static void addLifeTime( EntityID id, float time )
{
	LifeTimeData* currLife = NULL;

	if( ecps_GetComponentFromEntityByID( &gameECPS, id, lifeTimeCompID, &currLife ) ) {
		currLife->timeLeft = time;
	} else {
		LifeTimeData life;
		life.timeLeft = time;
		ecps_AddComponentToEntityByID( &gameECPS, id, lifeTimeCompID, &life );
	}
}

//*****************************************************************************************************

typedef struct {
	char c;
	bool pressed;
	EntityID display;
} CharacterToPress;

CharacterToPress* sbCurrSelection = NULL;

static bool resetAvailable = false;

static int whiteCircleImg;

static int numCharacters;
static int stage = 0;
static int level = 0;

static int font;

static float timeLeft;
static float timeAllotted;

static unsigned int score = 0;
static unsigned int highScore = 0;

static EntityID shieldDisplay;
static EntityID fistDisplay;

static int characterIdleImg;
static int characterActImg;
static int characterHurtImg;
static int characterDeadImg;

static int currentCharacterImg;

static bool inputEnabled;

static Sequence* currSequence = NULL;
static Sequence deathSequence;
static Sequence successSequence;
static Sequence failSequence;

static void createSmokeParticle( Vector2 basePos, Vector2 baseVelocity, int8_t depth )
{
	// we'll have it start dying immediately, have it shrink instead of fade
	GCPosData pos;
	GCScaleData scale;
	KinematicData physics;
	GCSpriteData sprite;
	GCColorData color;
	Vec2LerpData scaleLerp;
	ColorLerpData colorLerp;
	LifeTimeData life;

	pos.currPos = pos.futurePos = basePos;

	float baseScale = rand_GetToleranceFloat( NULL, 0.5f, 0.2f );
	scale.currScale = scale.futureScale = vec2( baseScale, baseScale );

	physics.vel = baseVelocity;
	physics.gravScale = vec2( 0.0f, -0.5f ); // go up instead of down

	sprite.camFlags = 1;
	sprite.depth = depth;
	sprite.img = whiteCircleImg;

	float val = rand_GetRangeFloat( NULL, 0.8f, 1.0f );
	color.currClr = color.futureClr = clr( 0.0f, val, val, 1.0f );

	float lifeTime = rand_GetToleranceFloat( NULL, 0.5f, 0.15f );

	scaleLerp.start = scale.currScale;
	vec2_Scale( &scale.currScale, 5.0f, &scaleLerp.target );
	scaleLerp.time = lifeTime;
	scaleLerp.timePassed = 0.0f;
	scaleLerp.ease = easeInExpo;

	colorLerp.start = color.currClr;
	colorLerp.target = color.currClr;
	colorLerp.target.a = 0.0f;
	colorLerp.time = lifeTime;
	colorLerp.timePassed = 0.0f;
	colorLerp.ease = easeInExpo;

	life.timeLeft = lifeTime + 0.05f;

	ecps_CreateEntity( &gameECPS, 8,
		gcPosCompID, &pos,
		gcScaleCompID, &scale,
		kinematicCompID, &physics,
		gcSpriteCompID, &sprite,
		gcClrCompID, &color,
		scaleLerpCompID, &scaleLerp,
		lifeTimeCompID, &life,
		colorLerpCompID, &colorLerp );
}

static void createBloodParticle( Vector2 basePos, Vector2 baseVelocity, int8_t depth )
{
	// we'll have it start dying immediately, have it shrink instead of fade
	GCPosData pos;
	GCScaleData scale;
	KinematicData physics;
	GCSpriteData sprite;
	GCColorData color;
	Vec2LerpData scaleLerp;
	LifeTimeData life;

	pos.currPos = pos.futurePos = basePos;

	float baseScale = rand_GetToleranceFloat( NULL, 0.5f, 0.2f );
	scale.currScale = scale.futureScale = vec2( baseScale, baseScale );

	physics.vel = baseVelocity;
	physics.gravScale = VEC2_ONE;

	sprite.camFlags = 1;
	sprite.depth = depth;
	sprite.img = whiteCircleImg;

	color.currClr = color.futureClr = clr( rand_GetRangeFloat( NULL, 0.8f, 1.0f ), 0.0f, 0.0f, 1.0f );

	float lifeTime = rand_GetToleranceFloat( NULL, 0.5f, 0.15f );

	scaleLerp.start = scale.currScale;
	scaleLerp.target = VEC2_ZERO;
	scaleLerp.time = lifeTime;
	scaleLerp.timePassed = 0.0f;
	scaleLerp.ease = easeInExpo;

	life.timeLeft = lifeTime + 0.05f;

	ecps_CreateEntity( &gameECPS, 7,
		gcPosCompID, &pos,
		gcScaleCompID, &scale,
		kinematicCompID, &physics,
		gcSpriteCompID, &sprite,
		gcClrCompID, &color,
		scaleLerpCompID, &scaleLerp,
		lifeTimeCompID, &life );
}

static const char* chooseWord( int numChars )
{
	switch( numChars ) {
	case 3:
		return threeCharWords[rand_GetRangeS32( NULL, 0, ARRAY_SIZE( threeCharWords ) - 1 )];
	case 4:
		return fourCharWords[rand_GetRangeS32( NULL, 0, ARRAY_SIZE( fourCharWords ) - 1 )];
	case 5:
		return fiveCharWords[rand_GetRangeS32( NULL, 0, ARRAY_SIZE( fiveCharWords ) - 1 )];
	case 6:
		return sixCharWords[rand_GetRangeS32( NULL, 0, ARRAY_SIZE( sixCharWords ) - 1 )];
	case 7:
		return sevenCharWords[rand_GetRangeS32( NULL, 0, ARRAY_SIZE( sevenCharWords ) - 1 )];
	case 8:
		return eightCharWords[rand_GetRangeS32( NULL, 0, ARRAY_SIZE( eightCharWords ) - 1 )];
	case 9:
		return nineCharWords[rand_GetRangeS32( NULL, 0, ARRAY_SIZE( nineCharWords ) - 1 )];
	default:
		return tenCharWords[rand_GetRangeS32( NULL, 0, ARRAY_SIZE( tenCharWords ) - 1 )];
	}
}

static void setupPlay( void )
{
	currentCharacterImg = characterIdleImg;

	for( size_t i = 0; i < sb_Count( sbCurrSelection ); ++i ) {
		ecps_DestroyEntityByID( &gameECPS, sbCurrSelection[i].display );
	}

	timeLeft = stages[stage].time;
	timeAllotted = timeLeft;
	sb_Clear( sbCurrSelection );

	float width = 80.0f * level;
	float left = 400.0f - ( width / 2.0f ) + 20.0f;

	const char* word = chooseWord( level );
	size_t strlen = SDL_strlen( word );
	for( size_t i = 0; i < strlen; ++i ) {
		CharacterToPress toPress;
		toPress.c = word[i];
		toPress.pressed = false;

		if( stages[stage].capitalize && ( rand_GetNormalizedFloat( NULL ) <= 0.2f ) ) {
			// replace with a capital letter
			toPress.c = (char)toupper( toPress.c );
		}

		if( stages[stage].replace && ( rand_GetNormalizedFloat( NULL ) <= 0.1f ) ) {
			// replace with a random symbol
			size_t symlen = SDL_strlen( symbolChars );
			toPress.c = symbolChars[rand_GetRangeS32( NULL, 0, symlen )];
		}

		// create the display entity
		GCPosData pos;
		GCSpriteData sprite;
		GCFloatVal0Data val0;
		GCColorData clrData;
		ColorLerpData clrLerp;

		sprite.img = txt_GetCharacterImage( font, toPress.c );
		sprite.depth = 100;
		sprite.camFlags = 1;

		val0.currValue = 1.0f;
		val0.futureValue = 1.0f;

		clrData.currClr = clrData.futureClr = CLR_WHITE;
		clrData.currClr.a = clrData.futureClr.a = 0.0f;

		clrLerp.ease = NULL;
		clrLerp.time = 0.1f;
		clrLerp.timePassed = 0.0f;
		clrLerp.start = clr( 1.0f, 1.0f, 1.0f, 0.0f );
		clrLerp.target = CLR_WHITE;

		pos.currPos = pos.futurePos = vec2( left + ( 80.0f * i ), 500.0f );

		toPress.display = ecps_CreateEntity( &gameECPS, 4,
			gcPosCompID, &pos,
			gcSpriteCompID, &sprite,
			gcClrCompID, &clrData,
			colorLerpCompID, &clrLerp );


		sb_Push( sbCurrSelection, toPress );
		//llog( LOG_DEBUG, "Adding character: %c   %i", toPress.c, (int)toPress.c );
	}
}

static void setupTutorial( void )
{
	resetAvailable = false;
	stage = 0;
	level = 3;
	score = 0;
	highScore = 0;
	currentCharacterImg = characterIdleImg;
	inputEnabled = true;
	currSequence = NULL;
	createEyes( );
	setupPlay( );

	addAlphaLerp( tutorialDisplay, 0.35f, 1.0f, NULL );
	addAlphaLerp( tutorialBG, 0.35f, 1.0f, NULL );

	addColorLerp( youWinDisplay, 0.75f, clr( 1.0f, 1.0f, 1.0f, 0.0f ), NULL );

	ignoreTime = true;
}

static void reset( )
{
	stage = 1;
	level = 4;
	score = 0;
	setupPlay( );
}

static void advanceLevel( )
{
	ignoreTime = false;

	++level;
	if( ( level > 10 ) || ( stage == 0 ) ) {
		level = 4;
		++stage;
		if( stage >= ARRAY_SIZE( stages ) ) {
			stage = ARRAY_SIZE( stages ) - 1;
		}
	}
	//llog( LOG_DEBUG, "New level: %i-%i", stage, level );
	setupPlay( );
}


static void playDeath( void )
{
	inputEnabled = false;
	sequence_Reset( &deathSequence );
	currSequence = &deathSequence;
}

static void playSuccess( void )
{
	inputEnabled = false;
	sequence_Reset( &successSequence );
	currSequence = &successSequence;
}

static void playFailure( void )
{
	inputEnabled = false;
	sequence_Reset( &failSequence );
	currSequence = &failSequence;
}

static float sequenceDone( void* data, bool* outDone )
{
	inputEnabled = true;
	currSequence = NULL;
	( *outDone ) = true;
	return 0.0f;
}

static float death_PunchDown( void* data, bool* outDone )
{
	addPosLerp( fistDisplay, 0.25f, vec2( 400.0f, 420.0f ), easeInQuint );

	( *outDone ) = true;
	return 0.25f;
}

static float death_PunchContact( void* data, bool* outDone )
{
	// pause and effects
	snd_Play( deathSound, 1.0f, 1.0f, 0.0f, 0 );
	Vector2 basePos = vec2( 400.0f, 410.0f );
	for( int i = rand_GetRangeS32( NULL, 10, 12 ); i >= 0; --i ) {
		Vector2 vel = vec2( rand_GetToleranceFloat( NULL, 0.0f, 500.0f ), rand_GetRangeFloat( NULL, -450.0f, -100.0f ) );
		createBloodParticle( basePos, vel, 100 );
	}

	currentCharacterImg = characterDeadImg;

	( *outDone ) = true;
	return 0.4f;
}

static float death_Withdraw( void* data, bool* outDone )
{
	Vector2 basePos = vec2( 400.0f, 410.0f );
	// create a few above to have blood fall off the fist
	for( int i = rand_GetRangeS32( NULL, 4, 8 ); i >= 0; --i ) {
		Vector2 offset = vec2( rand_GetRangeFloat( NULL, -40.0f, 40.0f ), rand_GetRangeFloat( NULL, -100.0f, -300.0f ) );
		vec2_Add( &basePos, &offset, &offset );
		createBloodParticle( offset, VEC2_ZERO, 50 );
	}

	addPosLerp( fistDisplay, 0.5f, vec2( 400.0f, -5.0f ), easeOutQuad );

	( *outDone ) = true;
	return 0.45f;
}

static float death_GameOver( void* data, bool* outDone )
{
	resetAvailable = true;
	addColorLerp( youWinDisplay, 0.75f, clr( 1.0f, 1.0f, 1.0f, 1.0f ), NULL );

	( *outDone ) = true;
	return 0.0f;
}

static void createDeathSequence( void )
{
	sequence_Init( &deathSequence, NULL, 4,
		death_PunchDown,
		death_PunchContact,
		death_Withdraw,
		death_GameOver );
}

static void playRandomRoar( void )
{
	snd_Play( sbRoarSounds[rand_GetRangeS32( NULL, 0, sb_Count( sbRoarSounds ) - 1 )], 0.75f, rand_GetToleranceFloat( NULL, 1.0f, 0.1f ), 0.0f, 0 );
}

static float success_PunchDown( void* data, bool* outDone )
{
	addPosLerp( fistDisplay, 0.15f, vec2( 400.0f, 325.0f ), easeInQuint );

	( *outDone ) = true;
	return 0.15f;
}

static float success_PunchContact( void* data, bool* outDone )
{
	snd_Play( blockSound, 1.0f, rand_GetToleranceFloat( NULL, 1.0f, 0.1f ), 0.0f, 0 );
	// pause and effects
	Vector2 basePos = vec2( 400.0f, 325.0f );
	for( int i = rand_GetRangeS32( NULL, 20, 30 ); i >= 0; --i ) {
		float xVel = rand_GetRangeFloat( NULL, 200.0f, 500.0f ) * sign( rand_GetRangeFloat( NULL, -1.0f, 1.0f ) );
		Vector2 vel = vec2( xVel, rand_GetRangeFloat( NULL, -10.0f, 100.0f ) );
		createSmokeParticle( basePos, vel, 100 );
	}

	( *outDone ) = true;
	return 0.2f;
}

static float success_Recoil( void* data, bool* outDone )
{
	playRandomRoar( );
	addColorLerp( shieldDisplay, 0.25f, clr( 0.0f, 1.0f, 1.0f, 0.0f ), NULL );
	addPosLerp( fistDisplay, 0.5f, vec2( 400.0f, -5.0f ), easeOutQuad );

	( *outDone ) = true;
	return 0.45f;
}

static float success_ChooseNewCharacters( void* data, bool* outDone )
{
	advanceLevel( );
	( *outDone ) = true;
	return 0.1f;
}

static void createSuccessSequence( void )
{
	sequence_Init( &successSequence, NULL, 5,
		success_PunchDown,
		success_PunchContact,
		success_Recoil,
		success_ChooseNewCharacters,
		sequenceDone );
}

static float failure_PunchDown( void* data, bool* outDone )
{
	addPosLerp( fistDisplay, 0.25f, vec2( 400.0f, 365.0f ), easeInQuint );

	( *outDone ) = true;
	return 0.25f;
}

static float failure_PunchContact( void* data, bool* outDone )
{
	snd_Play( breakSound, 1.0f, rand_GetToleranceFloat( NULL, 1.0f, 0.1f ), 0.0f, 0 );
	addColorLerp( shieldDisplay, 0.25f, clr( 0.0f, 1.0f, 1.0f, 0.0f ), NULL );
	currentCharacterImg = characterHurtImg;

	Vector2 basePos = vec2( 400.0f, 325.0f );
	for( int i = rand_GetRangeS32( NULL, 20, 30 ); i >= 0; --i ) {
		float xVel = rand_GetRangeFloat( NULL, 200.0f, 500.0f ) * sign( rand_GetRangeFloat( NULL, -1.0f, 1.0f ) );
		Vector2 vel = vec2( xVel, rand_GetRangeFloat( NULL, -10.0f, 100.0f ) );
		createSmokeParticle( basePos, vel, 100 );
	}

	( *outDone ) = true;
	return 0.2f;
}

static float failure_PunchWithdraw( void* data, bool* outDone )
{
	playRandomRoar( );
	addPosLerp( fistDisplay, 0.4f, vec2( 400.0f, -5.0f ), easeOutQuad );

	if( score > highScore ) {
		highScore = score;
	}

	( *outDone ) = true;
	return 0.35f;
}

static float failure_ChooseNewCharacters( void* data, bool* outDone )
{
	reset( );
	( *outDone ) = true;
	return 0.1f;
}

static void createFailureSequence( void )
{
	sequence_Init( &failSequence, NULL, 5,
		failure_PunchDown,
		failure_PunchContact,
		failure_PunchWithdraw,
		failure_ChooseNewCharacters,
		sequenceDone );
}

static int timeFiller0Img;
static int timeFiller1Img;
static int timeFiller2Img;
static int timeFiller3Img;
static int timeFillerBGImg;
static int platformImg;
static int whiteImg;
static GLuint whiteTextureID;

static int shieldImg;
static int fistImg;

static int* sbIrisImages = NULL;

static void createFist( void )
{
	GCPosData pos;
	GCSpriteData sprite;
	GCRotData rot;

	sprite.img = fistImg;
	sprite.depth = 52;
	sprite.camFlags = 1;

	// success pos = vec2( 400.0f, 325.0f );
	// failure pos = vec2( 400.0f, 365.0f );
	// death pos = vec2( 400.0f, 420.0f );
	pos.currPos = pos.futurePos = vec2( 400.0f, -5.0f );

	rot.currRot = rot.futureRot = 0.0f;

	fistDisplay = ecps_CreateEntity( &gameECPS, 3,
		gcPosCompID, &pos,
		gcSpriteCompID, &sprite,
		gcRotCompID, &rot );
}

static void createShield( void )
{
	GCPosData pos;
	GCSpriteData sprite;
	GCColorData clrData;

	sprite.img = shieldImg;
	sprite.depth = 51;
	sprite.camFlags = 1;

	clrData.currClr = clrData.futureClr = CLR_CYAN;
	clrData.currClr.a = clrData.futureClr.a = 0.0f;

	pos.currPos = pos.futurePos = vec2( 400.0f, 380.0f );

	shieldDisplay = ecps_CreateEntity( &gameECPS, 3,
		gcPosCompID, &pos,
		gcSpriteCompID, &sprite,
		gcClrCompID, &clrData );
}

typedef struct {
	EntityID pupilDisplay;
	EntityID whiteDisplay;
	Vector2 centerPos;
	float width;
	float currHeight;
	float futureHeight;
	float minHeight;
	float maxHeight;
	int8_t depth;
	float timeSinceLastMove;
	int stencilID;
} Eye;

static Eye* sbEyes = NULL;
static float desiredEyeActivity = 0.0f;
static float currEyeActivity = 0.0f;

static void processEyeActivity( float dt )
{
	if( currentCharacterImg == characterDeadImg ) {
		desiredRumbleVolume = 0.0f;
		for( size_t i = 0; i < sb_Count( sbEyes ); ++i ) {
			// height
			sbEyes[i].futureHeight = lerp( sbEyes[i].futureHeight, 0.0f, dt * 3.5f );
		}
	} else {
		currEyeActivity = lerp( currEyeActivity, desiredEyeActivity, dt * 5.0f );

		desiredRumbleVolume = lerp( 0.01f, 0.75f, currEyeActivity );

		float eyeMoveTime = lerp( 1.5f, 0.15f, currEyeActivity );
		float eyeMoveScale = lerp( 2.0f, 8.0f, currEyeActivity );
		for( size_t i = 0; i < sb_Count( sbEyes ); ++i ) {
			// height
			sbEyes[i].futureHeight = lerp( sbEyes[i].minHeight, sbEyes[i].maxHeight, currEyeActivity );

			// iris position
			sbEyes[i].timeSinceLastMove += dt + rand_GetRangeFloat( NULL, 0.0f, 0.1f );


			if( sbEyes[i].timeSinceLastMove >= eyeMoveTime ) {
				Vector2 newPos = vec2( rand_GetToleranceFloat( NULL, 0.0f, sbEyes[i].width / eyeMoveScale ), rand_GetToleranceFloat( NULL, 0.0f, sbEyes[i].minHeight / eyeMoveScale ) );
				vec2_Add( &( sbEyes[i].centerPos ), &newPos, &newPos );
				addPosLerp( sbEyes[i].pupilDisplay, rand_GetToleranceFloat( NULL, 0.5f, 0.25f ), newPos, easeOutQuart );
				sbEyes[i].timeSinceLastMove = 0.0f;
			}
		}
	}

	rumbleVolume = lerp( rumbleVolume, desiredRumbleVolume, 5.0f * dt );
	snd_SetVolume( rumbleVolume, 1 );
}

static EntityID createIris( Vector2 position, float scale, int8_t depth, int stencilID, int img )
{
	GCPosData pos;
	GCSpriteData sprite;
	GCStencilData stencil;
	GCScaleData scaleData;

	pos.currPos = pos.futurePos = position;

	sprite.camFlags = 1;
	sprite.depth = depth;
	sprite.img = img;

	stencil.isStencil = false;
	stencil.stencilID = stencilID;

	scaleData.currScale = scaleData.futureScale = vec2( scale, scale );

	return ecps_CreateEntity( &gameECPS, 4,
		gcPosCompID, &pos,
		gcScaleCompID, &scaleData,
		gcSpriteCompID, &sprite,
		gcStencilCompID, &stencil );
}

static EntityID createEyeWhites( Vector2 position, float size, int8_t depth, int stencilID, Color clr )
{
	GCPosData pos;
	GCScaleData scale;
	GCSpriteData sprite;
	GCStencilData stencil;
	GCColorData color;

	pos.currPos = pos.futurePos = position;

	sprite.camFlags = 1;
	sprite.depth = depth;
	sprite.img = whiteImg;

	Vector2 imgSize;
	img_GetSize( whiteImg, &imgSize );
	float imageScale = size / imgSize.y;
	scale.currScale = scale.futureScale = vec2( imageScale, imageScale );

	stencil.isStencil = false;
	stencil.stencilID = stencilID;

	color.currClr = color.futureClr = clr;

	return ecps_CreateEntity( &gameECPS, 5,
		gcPosCompID, &pos,
		gcScaleCompID, &scale,
		gcSpriteCompID, &sprite,
		gcStencilCompID, &stencil,
		gcClrCompID, &color );
}

static void createEye( Vector2 position, float width, float minHeight, float maxHeight, int8_t depth, int stencilID, int irisImg, Color eyeColor )
{
	Eye eye;

	eye.centerPos = position;
	eye.width = width;
	eye.currHeight = eye.futureHeight = minHeight;
	eye.minHeight = minHeight;
	eye.maxHeight = maxHeight;
	eye.depth = depth;
	eye.timeSinceLastMove = rand_GetToleranceFloat( NULL, 0.1f, 0.1f );
	eye.stencilID = stencilID;

	// create pupil
	float pupilScale = rand_GetToleranceFloat( NULL, width / 150.f, ( width / 150.f ) / 6.0f );
	eye.pupilDisplay = createIris( position, pupilScale, depth + 1, stencilID, irisImg );

	// create the whites
	eye.whiteDisplay = createEyeWhites( position, width, depth, stencilID, eyeColor );

	sb_Push( sbEyes, eye );
}

static void createEyes( void )
{
	for( size_t i = 0; i < sb_Count( sbEyes ); ++i ) {
		ecps_DestroyEntityByID( &gameECPS, sbEyes[i].pupilDisplay );
		ecps_DestroyEntityByID( &gameECPS, sbEyes[i].whiteDisplay );
	}
	sb_Clear( sbEyes );

	// divide the screen into five areas, one eye placed in each
	//  have some smaller ones scattered in the background as well
	Vector2 centerPos;
	float width;
	float minHeight;
	float maxHeight;

	int irisImg = sbIrisImages[rand_GetRangeS32( NULL, 0, sb_Count( sbIrisImages ) - 1 )];

	Color lightColors[] = {
		{ 0.85f, 0.85f, 0.85f, 1.0f },
		{ 0.85f, 0.0f, 0.0f, 1.0f },
		{ 0.3f, 0.0f, 0.4f, 1.0f },
		{ 0.5f, 0.5f, 0.5f, 1.0f }
	};

	Color darkColors[] = {
		{ 0.6f, 0.6f, 0.6f, 1.0f },
		{ 0.6f, 0.0f, 0.0f, 1.0f },
		{ 0.15f, 0.0f, 0.2f, 1.0f },
		{ 0.1f, 0.1f, 0.1f, 1.0f },
	};

	int clrIdx = rand_GetRangeS32( NULL, 0, ARRAY_SIZE( lightColors ) - 1 );

	// center eye
	centerPos.x = rand_GetToleranceFloat( NULL, 400.0f, 50.0f );
	centerPos.y = rand_GetToleranceFloat( NULL, 250.0f, 50.0f );
	width = rand_GetToleranceFloat( NULL, 150.0f, 20.0f );
	minHeight = rand_GetRangeFloat( NULL, width / 4.0f, width / 5.0f );
	maxHeight = rand_GetRangeFloat( NULL, width / 2.0f, width / 1.5f );
	createEye( centerPos, width, minHeight, maxHeight, 16, 0, irisImg, lightColors[clrIdx] );

	// top left eye
	centerPos.x = rand_GetToleranceFloat( NULL, 200.0f, 50.0f );
	centerPos.y = rand_GetToleranceFloat( NULL, 100.0f, 50.0f );
	width = rand_GetToleranceFloat( NULL, 125.0f, 20.0f );
	minHeight = rand_GetRangeFloat( NULL, width / 6.0f, width / 5.0f );
	maxHeight = rand_GetRangeFloat( NULL, width / 3.0f, width / 1.5f );
	createEye( centerPos, width, minHeight, maxHeight, 14, 0, irisImg, lightColors[clrIdx] );

	// bottom left eye
	centerPos.x = rand_GetToleranceFloat( NULL, 200.0f, 50.0f );
	centerPos.y = rand_GetToleranceFloat( NULL, 400.0f, 50.0f );
	width = rand_GetToleranceFloat( NULL, 125.0f, 20.0f );
	minHeight = rand_GetRangeFloat( NULL, width / 6.0f, width / 5.0f );
	maxHeight = rand_GetRangeFloat( NULL, width / 3.0f, width / 1.5f );
	createEye( centerPos, width, minHeight, maxHeight, 14, 0, irisImg, lightColors[clrIdx] );

	// top right eye
	centerPos.x = rand_GetToleranceFloat( NULL, 600.0f, 50.0f );
	centerPos.y = rand_GetToleranceFloat( NULL, 100.0f, 50.0f );
	width = rand_GetToleranceFloat( NULL, 125.0f, 20.0f );
	minHeight = rand_GetRangeFloat( NULL, width / 6.0f, width / 5.0f );
	maxHeight = rand_GetRangeFloat( NULL, width / 3.0f, width / 1.5f );
	createEye( centerPos, width, minHeight, maxHeight, 14, 0, irisImg, lightColors[clrIdx] );

	// bottom right eye
	centerPos.x = rand_GetToleranceFloat( NULL, 600.0f, 50.0f );
	centerPos.y = rand_GetToleranceFloat( NULL, 400.0f, 50.0f );
	width = rand_GetToleranceFloat( NULL, 125.0f, 20.0f );
	minHeight = rand_GetRangeFloat( NULL, width / 6.0f, width / 5.0f );
	maxHeight = rand_GetRangeFloat( NULL, width / 3.0f, width / 1.5f );
	createEye( centerPos, width, minHeight, maxHeight, 14, 0, irisImg, lightColors[clrIdx] );

	for( int randomEyes = rand_GetRangeS32( NULL, 4, 6 ); randomEyes > 0; --randomEyes ) {
		centerPos.x = rand_GetToleranceFloat( NULL, 400.0f, 375.0f );
		centerPos.y = rand_GetRangeFloat( NULL, 25.0f, 450.0f );
		width = rand_GetToleranceFloat( NULL, 50.0f, 20.0f );
		minHeight = rand_GetRangeFloat( NULL, width / 6.0f, width / 5.0f );
		maxHeight = rand_GetRangeFloat( NULL, width / 4.0f, width / 2.0f );
		createEye( centerPos, width, minHeight, maxHeight, 12, randomEyes, irisImg, darkColors[clrIdx] );
	}
}


#define LENS_SAMPLES 9
static void drawLens( Vector2 basePosition, float width, float height, int8_t depth, int stencilID )
{
	float halfHeight = height / 2.0f;

	Vector2 left = basePosition;
	left.x -= width / 2.0f;

	Vector2 right = basePosition;
	right.x += width / 2.0f;

	float step = width / LENS_SAMPLES;

	Vector2 bottomSamples[LENS_SAMPLES];
	Vector2 topSamples[LENS_SAMPLES];

	// starting at the left and going to the right
	bottomSamples[0] = left;
	bottomSamples[LENS_SAMPLES - 1] = right;

	topSamples[0] = left;
	topSamples[LENS_SAMPLES - 1] = right;

	for( int i = 1; i < ( LENS_SAMPLES - 1 ); ++i ) {
		float t = (float)i / ( LENS_SAMPLES - 1 );
		Vector2 basePos;
		vec2_Lerp( &left, &right, t, &basePos );

		bottomSamples[i] = basePos;
		topSamples[i] = basePos;

		// we're assuming we're only ever oriented with the width horizontal and the height vertical
		float foldedT = ( 1.0f - ( SDL_fabsf( t - 0.5f ) / 0.5f ) );
		float offset = easeOutQuad( foldedT ) * halfHeight; // fake it

		topSamples[i].y -= offset;
		bottomSamples[i].y += offset;
	}

	TriVert verts[3];
	verts[0].col = verts[1].col = verts[2].col = CLR_WHITE;
	verts[0].uv = verts[1].uv = verts[2].uv = VEC2_ZERO;

	// draw the left end cap
	verts[0].pos = left;
	verts[1].pos = bottomSamples[1];
	verts[2].pos = topSamples[1];
	triRenderer_AddVertices( verts, ST_DEFAULT, whiteTextureID, 0.0f, stencilID, 1, depth, TT_STENCIL );

	// draw the right end cap
	verts[0].pos = right;
	verts[1].pos = bottomSamples[LENS_SAMPLES - 2];
	verts[2].pos = topSamples[LENS_SAMPLES - 2];
	triRenderer_AddVertices( verts, ST_DEFAULT, whiteTextureID, 0.0f, stencilID, 1, depth, TT_STENCIL );

	// draw the center parts
	for( int i = 1; i < ( LENS_SAMPLES - 2 ); ++i ) {
		verts[0].pos = bottomSamples[i];
		verts[1].pos = topSamples[i];
		verts[2].pos = bottomSamples[i+1];
		triRenderer_AddVertices( verts, ST_DEFAULT, whiteTextureID, 0.0f, stencilID, 1, depth, TT_STENCIL );

		verts[0].pos = topSamples[i];
		verts[1].pos = topSamples[i + 1];
		verts[2].pos = bottomSamples[i + 1];
		triRenderer_AddVertices( verts, ST_DEFAULT, whiteTextureID, 0.0f, stencilID, 1, depth, TT_STENCIL );//*/
	}
}

static void drawAllLenses( float t )
{
	for( size_t i = 0; i < sb_Count( sbEyes ); ++i ) {
		drawLens( sbEyes[i].centerPos, sbEyes[i].width, lerp( sbEyes[i].currHeight, sbEyes[i].futureHeight, t ), sbEyes[i].depth, sbEyes[i].stencilID );
	}
}

static void clearLenses( void )
{
	// post draw upkeep
	for( size_t i = 0; i < sb_Count( sbEyes ); ++i ) {
		sbEyes[i].currHeight = sbEyes[i].futureHeight;
	}
}

typedef struct {
	float t;
	float xPos;
	float currYPos;
	float futureYPos;
} WaterColumn;

static WaterColumn* sbNearWater = NULL;
static WaterColumn* sbMidWater = NULL;
static WaterColumn* sbFarWater = NULL;

static float waterTime = 0.0f;

#define WATER_RESOLUTION 40

static void processWater( WaterColumn* sbWater, float basePos, float magnitude, float frequency, float waveLength )
{
	for( size_t i = 0; i < sb_Count( sbWater ); ++i ) {
		sbWater[i].futureYPos = basePos + SDL_sinf( ( waterTime / frequency ) + ( waveLength * sbWater[i].t ) ) * magnitude;
	}
}

static void processAllWater( float dt )
{
	waterTime += dt;
	processWater( sbNearWater, 550.0f, 25.0f, 0.15f, 25.0f );
	processWater( sbMidWater, 500.0f, 15.0f, 0.25f, 40.0f );
	processWater( sbFarWater, 450.0f, 5.0f, 0.45f, 80.0f );
}

static void createAllWater( )
{
	for( int i = 0; i < WATER_RESOLUTION; ++i ) {
		WaterColumn w;

		w.t = i / (float)( WATER_RESOLUTION - 1 );
		w.xPos = lerp( -5.0f, 805.0f, w.t );

		w.currYPos = w.futureYPos = 550.0f;
		sb_Push( sbNearWater, w );

		w.currYPos = w.futureYPos = 500.0f;
		sb_Push( sbMidWater, w );

		w.currYPos = w.futureYPos = 450.0f;
		sb_Push( sbFarWater, w );
	}
}

static void drawSomeWater( float t, WaterColumn* sbWater, Color color, int8_t depth )
{
	TriVert verts[3];
	verts[0].col = verts[1].col = verts[2].col = color;
	verts[0].uv = verts[1].uv = verts[2].uv = VEC2_ZERO;

	for( size_t i = 0; i < sb_Count( sbWater ) - 1; ++i ) {
		verts[0].pos = vec2( sbWater[i].xPos, lerp( sbWater[i].currYPos, sbWater[i].futureYPos, t ) );
		verts[1].pos = vec2( sbWater[i].xPos, 605.0f );
		verts[2].pos = vec2( sbWater[i+1].xPos, 605.0f );
		triRenderer_AddVertices( verts, ST_DEFAULT, whiteTextureID, 0.0f, -1, 1, depth, TT_SOLID );

		verts[0].pos = vec2( sbWater[i].xPos, lerp( sbWater[i].currYPos, sbWater[i].futureYPos, t ) );
		verts[1].pos = vec2( sbWater[i+1].xPos, lerp( sbWater[i + 1].currYPos, sbWater[i + 1].futureYPos, t ) );
		verts[2].pos = vec2( sbWater[i + 1].xPos, 605.0f );
		triRenderer_AddVertices( verts, ST_DEFAULT, whiteTextureID, 0.0f, -1, 1, depth, TT_SOLID );
	}
}

static void drawWater( float t )
{
	drawSomeWater( t, sbNearWater, clr( 0.15f, 0.15f, 0.18f, 1.0f ), 50 );
	drawSomeWater( t, sbMidWater, clr( 0.05f, 0.05f, 0.08f, 1.0f ), -2 );
	drawSomeWater( t, sbFarWater, clr( 0.025f, 0.025f, 0.028f, 1.0f ), -3 );
}

static void clearSomeWater( WaterColumn* sbWater )
{
	for( size_t i = 0; i < sb_Count( sbWater ); ++i ) {
		sbWater[i].currYPos = sbWater[i].futureYPos;
	}
}

static void clearWater( void )
{
	clearSomeWater( sbNearWater );
	clearSomeWater( sbMidWater );
	clearSomeWater( sbFarWater );
}

static void createYouWin( void )
{
	GCSpriteData sprite;
	GCPosData pos;
	GCColorData color;

	sprite.camFlags = 1;
	sprite.depth = 120;
	sprite.img = youWinImg;

	pos.currPos = pos.futurePos = vec2( 400.0f, 250.0f );

	Color c = clr( 1.0f, 1.0, 1.0, 0.0f );
	color.currClr = color.futureClr = c;

	youWinDisplay = ecps_CreateEntity( &gameECPS, 3,
		gcSpriteCompID, &sprite,
		gcPosCompID, &pos,
		gcClrCompID, &color );
}

static void createTutorial( void )
{
	GCPosData pos;
	GCSpriteData sprite;
	GCColorData color;

	pos.currPos = pos.futurePos = vec2( 400.0f, 200.0f );
	color.currClr = color.futureClr = CLR_WHITE;

	sprite.img = tutorialImg;
	sprite.depth = 100;
	sprite.camFlags = 1;

	tutorialDisplay = ecps_CreateEntity( &gameECPS, 3,
		gcPosCompID, &pos,
		gcSpriteCompID, &sprite,
		gcClrCompID, &color );
}

static void createTutorialBG( void )
{
	GCPosData pos;
	GCSpriteData sprite;
	GCColorData color;
	GCScaleData scale;

	pos.currPos = pos.futurePos = vec2( 400.0f, 200.0f );
	color.currClr = color.futureClr = clr( 0.1f, 0.1f, 0.1f, 0.95f );

	Vector2 tutorialSize;
	Vector2 whiteSize;
	img_GetSize( tutorialImg, &tutorialSize );
	img_GetSize( whiteImg, &whiteSize );

	tutorialSize.x += 20.0f;
	tutorialSize.y += 20.0f;

	Vector2 scaleAmt = vec2( tutorialSize.x / whiteSize.x, tutorialSize.y / whiteSize.y );
	scale.currScale = scale.futureScale = scaleAmt;

	sprite.img = whiteImg;
	sprite.depth = 99;
	sprite.camFlags = 1;

	tutorialBG = ecps_CreateEntity( &gameECPS, 4,
		gcPosCompID, &pos,
		gcSpriteCompID, &sprite,
		gcClrCompID, &color,
		gcScaleCompID, &scale );
}

static int gameScreen_Enter( void )
{
	cam_TurnOnFlags( 0, 1 );
	
	gfx_SetClearColor( CLR_BLACK );

	rumbleVolume = 0.0f;
	desiredRumbleVolume = 0.0f;
	snd_SetVolume( 1.0f, 0 );
	snd_SetVolume( 0.0f, 1 );

	font = txt_CreateSDFFont( "Fonts/CourierPrime-Regular.ttf" );

	whiteImg = img_Load( "Images/white.png", ST_DEFAULT );
	img_GetTextureID( whiteImg, &whiteTextureID );

	whiteCircleImg = img_Load( "Images/whiteCircle.png", ST_DEFAULT );

	timeFiller0Img = img_Load( "Images/vine1.png", ST_IMAGE_SDF );
	timeFiller1Img = img_Load( "Images/vine2.png", ST_IMAGE_SDF );
	timeFiller2Img = img_Load( "Images/vine3.png", ST_IMAGE_SDF );
	timeFiller3Img = img_Load( "Images/vine4.png", ST_IMAGE_SDF );
	timeFillerBGImg = img_Load( "Images/barbg3.png", ST_DEFAULT );
	platformImg = img_Load( "Images/platform.png", ST_DEFAULT );

	youWinImg = img_Load( "Images/youwin.png", ST_IMAGE_SDF );
	tutorialImg = img_Load( "Images/tutorial.png", ST_IMAGE_SDF );

	characterIdleImg = img_Load( "Images/character_idle.png", ST_DEFAULT );
	characterActImg = img_Load( "Images/character_act.png", ST_DEFAULT );
	characterHurtImg = img_Load( "Images/character_hurt.png", ST_DEFAULT );
	characterDeadImg = img_Load( "Images/character_dead.png", ST_DEFAULT );

	blockSound = snd_LoadSample( "Sounds/block.ogg", 1, false );
	breakSound = snd_LoadSample( "Sounds/break.ogg", 1, false );
	deathSound = snd_LoadSample( "Sounds/death.ogg", 1, false );

	sb_Push( sbRoarSounds, snd_LoadSample( "Sounds/roar0.ogg", 1, false ) );
	sb_Push( sbRoarSounds, snd_LoadSample( "Sounds/roar1.ogg", 1, false ) );
	sb_Push( sbRoarSounds, snd_LoadSample( "Sounds/roar2.ogg", 1, false ) );
	sb_Push( sbRoarSounds, snd_LoadSample( "Sounds/roar3.ogg", 1, false ) );

	droneStrm = snd_LoadStreaming( "Sounds/drone.ogg", true, 1 );
	doomStrm = snd_LoadStreaming( "Sounds/doom.ogg", true, 1 );

	snd_PlayStreaming( droneStrm, 1.0f, 0.0f );
	snd_PlayStreaming( doomStrm, 1.0f, 0.0f );

	sb_Push( sbIrisImages, img_Load( "Images/iris0.png", ST_DEFAULT ) );
	sb_Push( sbIrisImages, img_Load( "Images/iris1.png", ST_DEFAULT ) );
	sb_Push( sbIrisImages, img_Load( "Images/iris3.png", ST_DEFAULT ) );

	shieldImg = img_Load( "Images/shield2.png", ST_IMAGE_SDF );

	fistImg = img_Load( "Images/crusher.png", ST_DEFAULT );
	Vector2 fistSize;
	img_GetSize( fistImg, &fistSize );
	fistSize.x = 0.0f;
	fistSize.y /= -2.0f;
	img_SetOffset( fistImg, fistSize );

	ecps_StartInitialization( &gameECPS ); {
		gc_Register( &gameECPS );
		registerComponents( );

		gp_RegisterProcesses( &gameECPS );
		registerProcesses( );
	} ecps_FinishInitialization( &gameECPS );

	createShield( );
	createFist( );
	//createEyes( );
	createAllWater( );
	createYouWin( );
	createTutorial( );
	createTutorialBG( );

	createDeathSequence( );
	createSuccessSequence( );
	createFailureSequence( );

	SDL_StartTextInput( );

	inputEnabled = true;
	setupTutorial( );

	gfx_AddDrawTrisFunc( drawAllLenses );
	gfx_AddClearCommand( clearLenses );

	gfx_AddDrawTrisFunc( drawWater );
	gfx_AddClearCommand( clearWater );

	return 1;
}

static int gameScreen_Exit( void )
{
	return 1;
}

static void gameScreen_ProcessEvents( SDL_Event* e )
{
	if( resetAvailable ) {
		if( e->type == SDL_KEYDOWN ) {
			if( e->key.keysym.sym == SDLK_RETURN ) {
				// reset the game
				setupTutorial( );
			}
		}
	}

	if( !inputEnabled ) return;

	// grab any key press
	if( e->type == SDL_TEXTINPUT ) {
		char input = e->text.text[0];
		//llog( LOG_DEBUG, "Input: %s", e->text.text );

		bool validInput = false;
		for( size_t i = 0; ( i < sb_Count( sbCurrSelection ) ) && !validInput; ++i ) {
			if( !sbCurrSelection[i].pressed && ( sbCurrSelection[i].c == input ) ) {
				validInput = true;
				sbCurrSelection[i].pressed = true;

				addColorLerp( sbCurrSelection[i].display, 0.5f, clr( 0.0f, 1.0f, 1.0f, 0.0f ), NULL );
				addLifeTime( sbCurrSelection[i].display, 0.55f );

				sbCurrSelection[i].display = INVALID_ENTITY_ID;
			}
		}

		currentCharacterImg = characterActImg;
		if( validInput ) {
			size_t numLeft = sb_Count( sbCurrSelection );
			for( size_t i = 0; i < sb_Count( sbCurrSelection ); ++i ) {
				if( sbCurrSelection[i].pressed ) {
					--numLeft;
				}
			}

			addColorLerp( shieldDisplay, 0.1f, clr( 0.0f, 1.0f, 1.0f, lerp( 0.5f, 1.0f, 1.0f - ( (float)numLeft / (float)( sb_Count( sbCurrSelection ) ) ) ) ), NULL );

			//llog( LOG_DEBUG, "Pressed valid character %c", input );
			if( numLeft == 0 ) {
				//llog( LOG_DEBUG, "Advance level", input );
				timeLeft = 0.0f;
				ignoreTime = false;
				++score;
				if( score > highScore ) {
					highScore = score;
				}

				addAlphaLerp( tutorialDisplay, 0.5f, 0.0f, NULL );
				addAlphaLerp( tutorialBG, 0.5f, 0.0f, NULL );
			}
		} else {
			if( stage != 0 ) {
				//llog( LOG_DEBUG, "Pressed invalid character %c", input );
				timeLeft -= ( timeAllotted / 20.0f );
			}
		}
	}
}

static void gameScreen_Process( void )
{
	
}

static void gameScreen_Draw( void )
{
	ecps_RunProcess( &gameECPS, &gpRenderProc );

	// draw timer
	Vector2 pos = vec2( 400.0f, 70.0f );
	Color color = clr_byte( 168, 0, 32, 0 );
	float t = 1.0f - ( timeLeft / timeAllotted );
	t = remap( 0.0f, 1.0f, t, 0.1f, 1.0f );
	color.a = lerp( 0.48f, 1.0f, t * t );

	int fillDraw = img_CreateDraw( timeFiller0Img, 1, pos, pos, 100 );
	img_SetDrawColor( fillDraw, color, color );

	fillDraw = img_CreateDraw( timeFiller1Img, 1, pos, pos, 100 );
	img_SetDrawColor( fillDraw, color, color );

	fillDraw = img_CreateDraw( timeFiller2Img, 1, pos, pos, 100 );
	img_SetDrawColor( fillDraw, color, color );

	fillDraw = img_CreateDraw( timeFiller3Img, 1, pos, pos, 100 );
	img_SetDrawColor( fillDraw, color, color );

	img_CreateDraw( timeFillerBGImg, 1, pos, pos, 99 );

	// draw scores
	char scoreText[32];
	SDL_snprintf( scoreText, 31, "%i", score );
	txt_DisplayString( scoreText, vec2( 20.0f, 10.0f ), CLR_WHITE, HORIZ_ALIGN_LEFT, VERT_ALIGN_TOP, font, 1, 100, 64.0f );

	SDL_snprintf( scoreText, 31, "%i", highScore );
	txt_DisplayString( scoreText, vec2( 20.0f, 72.0f ), CLR_CYAN, HORIZ_ALIGN_LEFT, VERT_ALIGN_TOP, font, 1, 100, 32.0f );

	// draw environmental bits
	pos = vec2( 400.0f, 600.0f - ( 186.0f / 2.0f ) );
	img_CreateDraw( platformImg, 1, pos, pos, 50 );

	// draw the character
	pos = vec2( 400.0f, 390.0f );
	img_CreateDraw( currentCharacterImg, 1, pos, pos, 51 );
}

static void gameScreen_PhysicsTick( float dt )
{
	// game process
	if( currSequence != NULL ) {
		sequence_Run( currSequence, dt );
	} else {
		desiredEyeActivity = 1.0f - ( timeLeft / timeAllotted );
		if( !ignoreTime ) {
			timeLeft -= dt;
			if( timeLeft <= 0.0f ) {
				// trigger level advance
				//  three options here: they did nothing and die, they did something but didn't succeed, they succeeded
				if( currentCharacterImg == characterIdleImg ) {
					// they did nothing, they die
					playDeath( );
				} else {
					bool success = true;
					for( size_t i = 0; success && ( i < sb_Count( sbCurrSelection ) ); ++i ) {
						if( !sbCurrSelection[i].pressed ) {
							success = false;
						}
					}

					if( success ) {
						// they advance to the next stage
						playSuccess( );
					} else {
						// the game resets to the first stage
						playFailure( );
					}
				}
			}
		}
	}

	procDT = dt;
	ecps_RunProcess( &gameECPS, &val0LerpProc );
	ecps_RunProcess( &gameECPS, &colorLerpProc );
	ecps_RunProcess( &gameECPS, &posLerpProc );
	ecps_RunProcess( &gameECPS, &scaleLerpProc );
	ecps_RunProcess( &gameECPS, &lifeTimeProc );
	ecps_RunProcess( &gameECPS, &simplePhysicsProc );
	processEyeActivity( dt );
	processAllWater( dt );
}

GameState gameScreenState = { gameScreen_Enter, gameScreen_Exit, gameScreen_ProcessEvents,
	gameScreen_Process, gameScreen_Draw, gameScreen_PhysicsTick };