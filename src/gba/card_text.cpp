#include "gba/card_text.h"

#include "bn_sstream.h"

namespace gba
{

namespace
{

bn::string_view base_name(engine::CardId id)
{
    switch(id)
    {
    case engine::CardId::STRIKE:
        return "Frappe";
    case engine::CardId::DEFEND:
        return "Défense";
    case engine::CardId::BASH:
        return "Heurt";
    case engine::CardId::IRON_WAVE:
        return "Vague de fer";
    case engine::CardId::CLEAVE:
        return "Coup de tonnerre";
    case engine::CardId::ANGER:
        return "Colère";
    case engine::CardId::METALLICIZE:
        return "Armure de pierre";
    case engine::CardId::NEUTRALIZE:
        return "Neutralisation";
    case engine::CardId::SURVIVOR:
        return "Survivant";
    case engine::CardId::POISON_STAB:
        return "Poignard empoisonné";
    case engine::CardId::DEADLY_POISON:
        return "Poison mortel";
    case engine::CardId::ZAP:
        return "Éclair";
    case engine::CardId::DUALCAST:
        return "Activation double";
    case engine::CardId::FALLING_STAR:
        return "Chute d'étoile";
    case engine::CardId::VENERATE:
        return "Vénération";
    case engine::CardId::UNLEASH:
        return "Déchaînement";
    case engine::CardId::BODYGUARD:
        return "Garde du corps";
    case engine::CardId::TWIN_STRIKE:
        return "Frappe jumelée";
    case engine::CardId::POMMEL_STRIKE:
        return "Frappe du pommeau";
    case engine::CardId::SHRUG_IT_OFF:
        return "Même pas mal";
    case engine::CardId::TAUNT:
        return "Provocation";
    default:
        return "???";
    }
}

}

void card_name(const engine::DeckCard& card, bn::istring& out)
{
    out.clear();
    out.append(base_name(card.id));

    if(card.upgraded)
    {
        out.push_back('+');
    }
}

void card_description(const engine::DeckCard& card, bn::istring& out)
{
    out.clear();
    bn::ostringstream stream(out);
    int value = engine::CardDef::get_value(card.id, card.upgraded);
    int secondary = engine::CardDef::get_secondary(card.id, card.upgraded);

    switch(card.id)
    {
    case engine::CardId::STRIKE:
    case engine::CardId::UNLEASH:
        stream << "Inflige " << value << " dégâts.";
        break;
    case engine::CardId::DEFEND:
        stream << "Gagne " << value << " d'Armure.";
        break;
    case engine::CardId::BASH:
        stream << "Inflige " << value << " dégâts. Vulnérable " << secondary << ".";
        break;
    case engine::CardId::IRON_WAVE:
        stream << "Gagne " << secondary << " d'Armure. Inflige " << value << " dégâts.";
        break;
    case engine::CardId::CLEAVE:
        stream << "Inflige " << value << " dégâts à TOUS les ennemis.";
        break;
    case engine::CardId::ANGER:
        stream << "Inflige " << value << " dégâts. Copie dans la défausse.";
        break;
    case engine::CardId::METALLICIZE:
        stream << "Fin de tour : gagne " << value << " d'Armure.";
        break;
    case engine::CardId::NEUTRALIZE:
        stream << "Inflige " << value << " dégâts. Affaibli " << secondary << ".";
        break;
    case engine::CardId::SURVIVOR:
        stream << "Gagne " << value << " d'Armure. Défausse 1 carte.";
        break;
    case engine::CardId::POISON_STAB:
        stream << "Inflige " << value << " dégâts. " << secondary << " de Poison.";
        break;
    case engine::CardId::DEADLY_POISON:
        stream << "Applique " << secondary << " de Poison.";
        break;
    case engine::CardId::ZAP:
        stream << "Canalise " << secondary << " Orbe de foudre.";
        break;
    case engine::CardId::DUALCAST:
        stream << "Évoque votre Orbe la plus ancienne 2 fois.";
        break;
    case engine::CardId::FALLING_STAR:
        stream << "Inflige " << value << " dégâts. Gagne " << secondary << " Étoile(s).";
        break;
    case engine::CardId::VENERATE:
        stream << "Gagne " << value << " d'Armure et " << secondary << " Étoiles.";
        break;
    case engine::CardId::BODYGUARD:
        stream << "Gagne " << value << " d'Armure. Invoque Titos.";
        break;
    case engine::CardId::TWIN_STRIKE:
        stream << "Inflige " << value << " dégâts " << secondary << " fois.";
        break;
    case engine::CardId::POMMEL_STRIKE:
        stream << "Inflige " << value << " dégâts. Pioche " << secondary << (secondary > 1 ? " cartes." : " carte.");
        break;
    case engine::CardId::SHRUG_IT_OFF:
        stream << "Gagne " << value << " d'Armure. Pioche " << secondary << (secondary > 1 ? " cartes." : " carte.");
        break;
    case engine::CardId::TAUNT:
        stream << "Gagne " << value << " d'Armure. Vulnérable " << secondary << ".";
        break;
    default:
        break;
    }
}

bn::string_view card_type_name(engine::CardType type)
{
    switch(type)
    {
    case engine::CardType::ATTACK:
        return "Attaque";
    case engine::CardType::SKILL:
        return "Compétence";
    default:
        return "Pouvoir";
    }
}

void status_line(const engine::CombatEntity& entity, bn::istring& out)
{
    out.clear();
    bn::ostringstream stream(out);

    for(const engine::StatusEffect& status : entity.statuses)
    {
        if(status.amount == 0)
        {
            continue;
        }

        if(! out.empty())
        {
            stream << ' ';
        }

        switch(status.type)
        {
        case engine::StatusType::STRENGTH:
            stream << "Force" << (status.amount > 0 ? "+" : "") << status.amount;
            break;
        case engine::StatusType::DEXTERITY:
            stream << "Dex" << (status.amount > 0 ? "+" : "") << status.amount;
            break;
        case engine::StatusType::VULNERABLE:
            stream << "Vuln" << status.amount;
            break;
        case engine::StatusType::WEAK:
            stream << "Faible" << status.amount;
            break;
        case engine::StatusType::POISON:
            stream << "Poison" << status.amount;
            break;
        case engine::StatusType::METALLICIZE:
            stream << "Métal" << status.amount;
            break;
        case engine::StatusType::PLATED_ARMOR:
            stream << "Plaque" << status.amount;
            break;
        case engine::StatusType::ARTIFACT:
            stream << "Artef" << status.amount;
            break;
        case engine::StatusType::RITUAL:
            stream << "Rituel" << status.amount;
            break;
        default:
            break;
        }
    }
}

}
