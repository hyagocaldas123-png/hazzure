// Exact installed model path to its exported mesh/material/idle animation.
#pragma once
namespace HazzureModelPreview {
struct CatalogueEntry {const char* model;const char* asset;};
inline constexpr CatalogueEntry Catalogue[]={
    {"agents/models/ctm_fbi/ctm_fbi_variantb.vmdl","models/agents/ct_1.glb"},
    {"agents/models/ctm_st6/ctm_st6_varianti.vmdl","models/agents/ct_2.glb"},
    {"agents/models/ctm_diver/ctm_diver_varianta.vmdl","models/agents/ct_3.glb"},
    {"agents/models/ctm_diver/ctm_diver_variantb.vmdl","models/agents/ct_4.glb"},
    {"agents/models/ctm_gendarmerie/ctm_gendarmerie_varianta.vmdl","models/agents/ct_5.glb"},
    {"agents/models/ctm_sas/ctm_sas_variantg.vmdl","models/agents/ct_6.glb"},
    {"agents/models/ctm_sas/ctm_sas_variantf.vmdl","models/agents/ct_7.glb"},
    {"agents/models/ctm_fbi/ctm_fbi_varianth.vmdl","models/agents/ct_8.glb"},
    {"agents/models/ctm_swat/ctm_swat_variante.vmdl","models/agents/ct_9.glb"},
    {"agents/models/ctm_st6/ctm_st6_variantj.vmdl","models/agents/ct_10.glb"},
    {"agents/models/tm_professional/tm_professional_varf.vmdl","models/agents/t_1.glb"},
    {"agents/models/tm_professional/tm_professional_varf1.vmdl","models/agents/t_2.glb"},
    {"agents/models/tm_professional/tm_professional_varf2.vmdl","models/agents/t_3.glb"},
    {"agents/models/tm_professional/tm_professional_varf3.vmdl","models/agents/t_4.glb"},
    {"agents/models/tm_professional/tm_professional_vari.vmdl","models/agents/t_5.glb"},
    {"agents/models/tm_professional/tm_professional_varg.vmdl","models/agents/t_6.glb"},
    {"agents/models/tm_professional/tm_professional_varj.vmdl","models/agents/t_7.glb"},
    {"agents/models/tm_balkan/tm_balkan_varianth.vmdl","models/agents/t_8.glb"},
    {"agents/models/tm_balkan/tm_balkan_variantg.vmdl","models/agents/t_9.glb"},
    {"agents/models/tm_balkan/tm_balkan_varianti.vmdl","models/agents/t_10.glb"},
    {"agents/models/tm_leet/tm_leet_variantf.vmdl","models/agents/t_11.glb"},
    {"agents/models/tm_leet/tm_leet_varianti.vmdl","models/agents/t_12.glb"},
    {"agents/models/tm_leet/tm_leet_varianth.vmdl","models/agents/t_13.glb"},
    {"agents/models/tm_phoenix/tm_phoenix_variantg.vmdl","models/agents/t_14.glb"},
    {"agents/models/tm_jungle_raider/tm_jungle_raider_variantf2.vmdl","models/agents/t_15.glb"},
};
inline const char* FindModel(const char* model){if(!model||!*model)return nullptr;for(const auto& item:Catalogue)if(!strcmp(item.model,model))return item.asset;return nullptr;}
}
