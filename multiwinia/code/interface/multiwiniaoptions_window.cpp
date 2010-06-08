#include "lib/universal_include.h"

#ifdef LOCATION_EDITOR

#include "lib/hi_res_time.h"
#include "lib/resource.h"
#include "lib/language_table.h"
#include "lib/text_renderer.h"

#include "interface/multiwiniaoptions_window.h"
#include "interface/checkbox.h"
#include "interface/drop_down_menu.h"
#include "interface/input_field.h"

#include "worldobject/gunturret.h"

#include "app.h"
#include "location_editor.h"
#include "level_file.h"
#include "renderer.h"
#include "location.h"
#include "global_world.h"
#include "multiwinia.h"

class TextTab : public DarwiniaButton
{
public:
    UnicodeString m_string;

    TextTab( UnicodeString string )
    :   DarwiniaButton(),
        m_string(string)
    {
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        float y = 7.5 + realY + (m_h - m_fontSize) / 2;
        glColor3f( 255, 255, 255 );
        g_editorFont.DrawText2D( realX+5, y, 12, m_string );
    }
};

class AttackersButton : public DarwiniaButton
{
public:
    int m_teamId;

    AttackersButton( int _teamId )
    :   DarwiniaButton(),
        m_teamId( _teamId )
    {
    }

    void MouseUp()
    {
        //if( m_inactive ) return;

        bool attacking = g_app->m_location->m_levelFile->m_attacking[m_teamId];
        if( attacking )
        {
            g_app->m_location->m_levelFile->m_attacking[m_teamId] = false;
        }
        else
        {
            if( !m_inactive )
            {
                g_app->m_location->m_levelFile->m_attacking[m_teamId] = true;
            }
        }
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        bool attacking =  g_app->m_location->m_levelFile->m_attacking[m_teamId];
        DarwiniaButton::Render( realX, realY, highlighted, clicked || attacking );

        bool noPlay = m_teamId >= g_app->m_location->m_levelFile->m_numPlayers;
        m_inactive = ( !attacking && g_app->m_location->GetNumAttackers() >= 2) || noPlay;

        if( m_inactive || m_teamId == 255 || m_teamId == -1 )
        {
            glColor3ub( 100, 100, 100 );
        }
        else
        {
            RGBAColour col = g_app->m_multiwinia->GetColour(m_teamId);
            glColor3ubv( col.GetData() );
        }

        glBegin( GL_QUADS );
            glVertex2i( realX + 5, realY + 4 );
            glVertex2i( realX + (m_w-5), realY + 4 );
            glVertex2i( realX + (m_w-5), realY + (m_h-4) );
            glVertex2i( realX + 5, realY + (m_h-4) );
        glEnd();
    }
};

class CoopGroupButton : public DarwiniaButton
{
public:
    int m_teamId;
    int m_groupId;

    CoopGroupButton( int _teamId, int _groupId )
    :   DarwiniaButton(),
        m_teamId(_teamId),
        m_groupId(_groupId)
    {
    }

    bool IsInGroup()
    {
        LList<int> *groups = NULL;
        if( m_groupId == 1 ) groups = &g_app->m_location->m_levelFile->m_coopGroup1Positions;
        else groups = &g_app->m_location->m_levelFile->m_coopGroup2Positions;

        for( int i = 0; i < groups->Size(); ++i )
        {
            if( groups->GetData(i) == m_teamId )
            {
                return true;
            }
        }

        return false;
    }

    int GetNumInGroup()
    {
        LList<int> *groups = NULL;
        if( m_groupId == 1 ) groups = &g_app->m_location->m_levelFile->m_coopGroup1Positions;
        else groups = &g_app->m_location->m_levelFile->m_coopGroup2Positions;

        return groups->Size();
    }

    bool IsInOtherGroup()
    {
        LList<int> *groups = NULL;
        if( m_groupId == 2 ) groups = &g_app->m_location->m_levelFile->m_coopGroup1Positions;
        else groups = &g_app->m_location->m_levelFile->m_coopGroup2Positions;

        for( int i = 0; i < groups->Size(); ++i )
        {
            if( groups->GetData(i) == m_teamId )
            {
                return true;
            }
        }

        return false;
    }

    void RemoveFromGroup()
    {
        LList<int> *groups = NULL;
        if( m_groupId == 1 ) groups = &g_app->m_location->m_levelFile->m_coopGroup1Positions;
        else groups = &g_app->m_location->m_levelFile->m_coopGroup2Positions;

        for( int i = 0; i < groups->Size(); ++i )
        {
            if( groups->GetData(i) == m_teamId )
            {
                groups->RemoveData(i);
                break;
            }
        }
    }

    void MouseUp()
    {
        if( m_teamId == -1 || m_groupId == -1 ) return;

        if( !IsInGroup() )
        {
            if( m_inactive ) return;
            if( m_groupId == 1 )
            {
                g_app->m_location->m_levelFile->m_coopGroup1Positions.PutData(m_teamId);
            }
            else
            {
                g_app->m_location->m_levelFile->m_coopGroup2Positions.PutData(m_teamId);
            }
        }
        else
        {
            RemoveFromGroup();
        }
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        DarwiniaButton::Render( realX, realY, highlighted , clicked || IsInGroup() );

        bool noPlay = m_teamId >= g_app->m_location->m_levelFile->m_numPlayers;
        m_inactive = (!IsInGroup() && GetNumInGroup() >= 2 ) || IsInOtherGroup() || noPlay;

        if( m_inactive || m_teamId == 255 || m_teamId == -1 )
        {
            glColor3ub( 100, 100, 100 );
        }
        else
        {
            RGBAColour col = g_app->m_multiwinia->GetColour(m_teamId);
            glColor3ubv( col.GetData() );
        }

        glBegin( GL_QUADS );
            glVertex2i( realX + 5, realY + 4 );
            glVertex2i( realX + (m_w-5), realY + 4 );
            glVertex2i( realX + (m_w-5), realY + (m_h-4) );
            glVertex2i( realX + 5, realY + (m_h-4) );
        glEnd();
    }
};

MultiwiniaOptionsWindow::MultiwiniaOptionsWindow()
:   DarwiniaWindow("multiwiniaoptions_window")
{
}

void MultiwiniaOptionsWindow::Create()
{
    DarwiniaWindow::Create();

	int height = 5;
	int pitch = 17;
	int buttonWidth = m_w - 20;

    TextTab *text = new TextTab(LANGUAGEPHRASE("editor_basic"));
    text->SetProperties( "editor_basic", 10, height+=pitch, 120, 15, UnicodeString() );
    RegisterButton( text );

	InputField *button = new InputField();
	button->SetShortProperties("map_name", 10, height += pitch, m_w - 20,15,LANGUAGEPHRASE("dialog_name"));
	button->RegisterString(g_app->m_location->m_levelFile->m_levelName, 32 );
	RegisterButton(button);

    button = new InputField();
	button->SetShortProperties("map_description", 10, height += pitch, m_w - 20,15,LANGUAGEPHRASE("editor_description"));
    button->RegisterString(g_app->m_location->m_levelFile->m_description, 512 );
	RegisterButton(button);

    DropDownMenu *gametype = new DropDownMenu();
    gametype->SetShortProperties( "gametype", 10, height+=pitch, m_w - 20, 15, LANGUAGEPHRASE("multiwinia_gametype") );
    for( int i = 0; i < Multiwinia::NumGameTypes; ++i )
    {
        if( g_app->IsGameModePermitted(i) )
        {
            gametype->AddOption( LANGUAGEPHRASE(Multiwinia::s_gameBlueprints[i]->GetName()), i );
        }
    }
    gametype->RegisterInt( &g_app->m_location->m_levelFile->m_gameTypes );
    RegisterButton(gametype);

	CreateValueControl( "editor_popcap", &g_app->m_location->m_levelFile->m_populationCap, height+=pitch, 10, 1, 5000 );
    CreateValueControl( "editor_numplayers", &g_app->m_location->m_levelFile->m_numPlayers, height+=pitch, 1, 1, 4 );
    CreateValueControl( "dialog_playtime", &g_app->m_location->m_levelFile->m_defaultPlayTime, height+=pitch, 1, 5, 30 );

    height+=pitch;

    text = new TextTab(LANGUAGEPHRASE("editor_coop_options"));
    text->SetProperties( "editor_coop_options", 10, height+=pitch, 120, 15, UnicodeString() );
    RegisterButton( text );
    CheckBox *coop = new CheckBox();
    coop->RegisterBool(&g_app->m_location->m_levelFile->m_coop);
    coop->SetShortProperties("editor_coop", 10, height+=pitch, m_w - 20, 15, LANGUAGEPHRASE("editor_coop") );
    RegisterButton( coop );

    coop = new CheckBox();
    coop->RegisterBool(&g_app->m_location->m_levelFile->m_forceCoop);
    coop->SetShortProperties("editor_forcecoop", 10, height+=pitch, m_w - 20, 15, LANGUAGEPHRASE("editor_forcecoop") );
    RegisterButton( coop );

    text = new TextTab(LANGUAGEPHRASE("editor_coopgroup1"));
    text->SetProperties( "editor_coopgroup1", 10, height+=pitch, 120, 15, UnicodeString() );
    RegisterButton( text );
    for( int i = 0; i < 4; ++i )
    {
        char name[64];
        sprintf( name, "coop_group_1_t%d", i );
        CoopGroupButton *cgb = new CoopGroupButton( i, 1 );
        cgb->SetShortProperties( name, 135 + i * 40, height, 35, 15, UnicodeString() );
        RegisterButton( cgb );
        cgb->m_caption = UnicodeString();
    }

    text = new TextTab(LANGUAGEPHRASE("editor_coopgroup2"));
    text->SetProperties( "texttab2", 10, height+=pitch, 120, 15, UnicodeString() );
    RegisterButton( text );
    for( int i = 0; i < 4; ++i )
    {
        char name[64];
        sprintf( name, "coop_group_2_t%d", i );
        CoopGroupButton *cgb = new CoopGroupButton( i, 2 );
        cgb->SetShortProperties( name, 135 + i * 40, height, 35, 15, UnicodeString() );
        RegisterButton( cgb );
        cgb->m_caption = UnicodeString();
    }

    height+=pitch;

    text = new TextTab(LANGUAGEPHRASE("editor_assault_options"));
    text->SetProperties( "editor_assault_options", 10, height+=pitch, 120, 15, UnicodeString() );
    RegisterButton( text );
    CreateValueControl( "editor_defenderpopcap", &g_app->m_location->m_levelFile->m_defenderPopulationCap, height+=pitch, 10, 50, 5000 );

    text = new TextTab(LANGUAGEPHRASE("editor_attackingteams"));
    text->SetProperties( "editor_attackingteams", 10, height+=pitch, 120, 15, UnicodeString() );
    RegisterButton( text );
    for( int i = 0; i < 4; ++i )
    {
        char name[64];
        sprintf( name, "attacking_team_t%d", i );
        AttackersButton *attackers = new AttackersButton( i );
        attackers->SetShortProperties( name, 135 + i * 40, height, 35, 15, UnicodeString() );
        RegisterButton( attackers );
        attackers->m_caption = UnicodeString();
    }

    height+=pitch;

    text = new TextTab(LANGUAGEPHRASE("editor_map_options"));
    text->SetProperties( "editor_map_options", 10, height+=pitch, 120, 15, UnicodeString() );
    RegisterButton( text );
    for( int i = 0; i < LevelFile::NumLevelOptions; ++i )
    {
        char name[256];
        sprintf( name, "editor_leveloption_%d", i );

        int min = 0, max = 1, change = 1;
        bool noControl = false;

        switch(i)
        {
            case LevelFile::LevelOptionNoHandicap:
            case LevelFile::LevelOptionInstantSpawnPointCapture:
            case LevelFile::LevelOptionNoGunTurrets:
            case LevelFile::LevelOptionForceRadarTeamMatch:
            case LevelFile::LevelOptionNoRadarControl:
            {
                CheckBox *cb = new CheckBox();
                cb->RegisterInt(&g_app->m_location->m_levelFile->m_levelOptions[i]);
                cb->SetShortProperties(name, 10, height+=pitch, m_w - 20, 15, LANGUAGEPHRASE(name) );
                RegisterButton( cb );

                noControl = true;
                break;
            }

            case LevelFile::LevelOptionDefenderReinforcementType:   
            case LevelFile::LevelOptionAttackerReinforcementType:
            {
                DropDownMenu *dropdown = new DropDownMenu();
                dropdown->SetShortProperties( name, 10, height+=pitch, m_w - 20, 15, UnicodeString(name) );
                dropdown->AddOption( GlobalResearch::GetTypeName(GlobalResearch::TypeAirStrike),     GlobalResearch::TypeAirStrike );
                dropdown->AddOption( GlobalResearch::GetTypeName(GlobalResearch::TypeAntsNest),      GlobalResearch::TypeAntsNest );
                dropdown->AddOption( GlobalResearch::GetTypeName(GlobalResearch::TypeArmour),        GlobalResearch::TypeArmour );
                dropdown->AddOption( GlobalResearch::GetTypeName(GlobalResearch::TypeBooster),       GlobalResearch::TypeBooster );
                dropdown->AddOption( GlobalResearch::GetTypeName(GlobalResearch::TypeDarkForest),    GlobalResearch::TypeDarkForest );
                dropdown->AddOption( GlobalResearch::GetTypeName(GlobalResearch::TypeEggSpawn),      GlobalResearch::TypeEggSpawn );
                dropdown->AddOption( GlobalResearch::GetTypeName(GlobalResearch::TypeEngineer),      GlobalResearch::TypeEngineer );
                dropdown->AddOption( GlobalResearch::GetTypeName(GlobalResearch::TypeGunTurret),     GlobalResearch::TypeGunTurret );
                dropdown->AddOption( GlobalResearch::GetTypeName(GlobalResearch::TypeHarvester),     GlobalResearch::TypeHarvester );
                dropdown->AddOption( GlobalResearch::GetTypeName(GlobalResearch::TypeHotFeet),       GlobalResearch::TypeHotFeet );
                dropdown->AddOption( GlobalResearch::GetTypeName(GlobalResearch::TypeMagicalForest), GlobalResearch::TypeMagicalForest );
                dropdown->AddOption( GlobalResearch::GetTypeName(GlobalResearch::TypeMeteorShower),  GlobalResearch::TypeMeteorShower );
                dropdown->AddOption( GlobalResearch::GetTypeName(GlobalResearch::TypeNuke),          GlobalResearch::TypeNuke );
                dropdown->AddOption( GlobalResearch::GetTypeName(GlobalResearch::TypeRage),          GlobalResearch::TypeRage );
                dropdown->AddOption( GlobalResearch::GetTypeName(GlobalResearch::TypeSquad),         GlobalResearch::TypeSquad );
                dropdown->AddOption( GlobalResearch::GetTypeName(GlobalResearch::TypeSubversion),    GlobalResearch::TypeSubversion );
                dropdown->RegisterInt( &g_app->m_location->m_levelFile->m_levelOptions[i] );
                RegisterButton( dropdown );

                noControl = true;
                break;
            }

            case LevelFile::LevelOptionAttackReinforcementMode:     
            case LevelFile::LevelOptionDefendReinforcementMode:
            {
                DropDownMenu *dropdown = new DropDownMenu();
                dropdown->SetShortProperties( name, 10, height+=pitch, m_w - 20, 15, UnicodeString(name) );
                dropdown->AddOption( LANGUAGEPHRASE("cratename_airstrike"), 0 );
                dropdown->AddOption( LANGUAGEPHRASE("cratename_napalmstrike"), 1 );
                dropdown->AddOption( LANGUAGEPHRASE("buildingname_flameturret"), GunTurret::GunTurretTypeFlame );
                dropdown->AddOption( LANGUAGEPHRASE("buildingname_rocketturret"), GunTurret::GunTurretTypeRocket );
                dropdown->AddOption( LANGUAGEPHRASE("buildingname_gunturret"), GunTurret::GunTurretTypeStandard );
                dropdown->RegisterInt( &g_app->m_location->m_levelFile->m_levelOptions[i] );
                RegisterButton( dropdown );

                noControl = true;
                break;
            }

            case LevelFile::LevelOptionTrunkPortReinforcements:     min = 0; max = 300; change = 10;    break;
            case LevelFile::LevelOptionTrunkPortArmour:             min = 0; max = 5;   change = 1;     break;
            case LevelFile::LevelOptionAttackerReinforcements:      min = 0; max = 300; change = 10;    break;
            case LevelFile::LevelOptionDefenderReinforcements:      min = 0; max = 300; change = 10;    break;
            case LevelFile::LevelOptionAirstrikeCaptureZones:       min = 0; max = 20;  change = 1;     break;
            case LevelFile::LevelOptionFuturewinianTeam:            min = 0; max = 3;   change = 1;     break;
        };

        if( !noControl )
        {
            CreateValueControl( name, &g_app->m_location->m_levelFile->m_levelOptions[i], height+=pitch, change, min, max );
        }
    }

    height+=pitch;

    text = new TextTab(LANGUAGEPHRASE("editor_effects"));
    text->SetProperties( "editor_effects", 10, height+=pitch, 120, 15, UnicodeString() );
    RegisterButton( text );

    CheckBox *effects = new CheckBox();
    effects->RegisterBool(&g_app->m_location->m_levelFile->m_effects[LevelFile::LevelEffectSnow]);
    effects->SetShortProperties("editor_effects_snow", 10, height+=pitch, m_w - 20, 15, LANGUAGEPHRASE("editor_effects_snow") );
    RegisterButton( effects );

    effects = new CheckBox();
    effects->RegisterBool(&g_app->m_location->m_levelFile->m_effects[LevelFile::LevelEffectLightning]);
    effects->SetShortProperties("editor_effects_lightning", 10, height+=pitch, m_w - 20, 15, LANGUAGEPHRASE("editor_effects_lightning") );
    RegisterButton( effects );

    effects = new CheckBox();
    effects->RegisterBool(&g_app->m_location->m_levelFile->m_effects[LevelFile::LevelEffectDustStorm]);
    effects->SetShortProperties("editor_effects_dustwind", 10, height+=pitch, m_w - 20, 15, LANGUAGEPHRASE("editor_effects_dustwind") );
    RegisterButton( effects );

    effects = new CheckBox();
    effects->RegisterBool(&g_app->m_location->m_levelFile->m_effects[LevelFile::LevelEffectSoulRain]);
    effects->SetShortProperties("editor_effects_soulrain", 10, height+=pitch, m_w - 20, 15, LANGUAGEPHRASE("editor_effects_soulrain") );
    RegisterButton( effects );

    effects = new CheckBox();
    effects->RegisterBool(&g_app->m_location->m_levelFile->m_effects[LevelFile::LevelEffectSpecialLighting]);
    effects->SetShortProperties("editor_effects_speciallights", 10, height+=pitch, m_w - 20, 15, LANGUAGEPHRASE("editor_effects_speciallights") );
    RegisterButton( effects );


}

#endif // LOCATION_EDITOR
