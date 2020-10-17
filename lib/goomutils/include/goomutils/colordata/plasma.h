#pragma once

#include "vivid/types.h"
#include <vector>

namespace goom::utils::colordata
{

// clang-format off
static const std::vector<vivid::srgb_t> plasma
{
  {   0.05038f,   0.02980f,   0.52797f },
  {   0.06354f,   0.02843f,   0.53312f },
  {   0.07535f,   0.02721f,   0.53801f },
  {   0.08622f,   0.02612f,   0.54266f },
  {   0.09638f,   0.02516f,   0.54710f },
  {   0.10598f,   0.02431f,   0.55137f },
  {   0.11512f,   0.02356f,   0.55547f },
  {   0.12390f,   0.02288f,   0.55942f },
  {   0.13238f,   0.02226f,   0.56325f },
  {   0.14060f,   0.02169f,   0.56696f },
  {   0.14861f,   0.02115f,   0.57056f },
  {   0.15642f,   0.02065f,   0.57407f },
  {   0.16407f,   0.02017f,   0.57748f },
  {   0.17157f,   0.01971f,   0.58081f },
  {   0.17895f,   0.01925f,   0.58405f },
  {   0.18621f,   0.01880f,   0.58723f },
  {   0.19337f,   0.01835f,   0.59033f },
  {   0.20045f,   0.01790f,   0.59336f },
  {   0.20744f,   0.01744f,   0.59633f },
  {   0.21435f,   0.01697f,   0.59924f },
  {   0.22120f,   0.01650f,   0.60208f },
  {   0.22798f,   0.01601f,   0.60487f },
  {   0.23472f,   0.01550f,   0.60759f },
  {   0.24140f,   0.01498f,   0.61026f },
  {   0.24803f,   0.01444f,   0.61287f },
  {   0.25463f,   0.01388f,   0.61542f },
  {   0.26118f,   0.01331f,   0.61791f },
  {   0.26770f,   0.01272f,   0.62035f },
  {   0.27419f,   0.01211f,   0.62272f },
  {   0.28065f,   0.01149f,   0.62504f },
  {   0.28708f,   0.01086f,   0.62730f },
  {   0.29348f,   0.01021f,   0.62949f },
  {   0.29985f,   0.00956f,   0.63162f },
  {   0.30621f,   0.00890f,   0.63369f },
  {   0.31254f,   0.00824f,   0.63570f },
  {   0.31886f,   0.00758f,   0.63764f },
  {   0.32515f,   0.00692f,   0.63951f },
  {   0.33143f,   0.00626f,   0.64132f },
  {   0.33768f,   0.00562f,   0.64305f },
  {   0.34392f,   0.00499f,   0.64471f },
  {   0.35015f,   0.00438f,   0.64630f },
  {   0.35636f,   0.00380f,   0.64781f },
  {   0.36255f,   0.00324f,   0.64924f },
  {   0.36873f,   0.00272f,   0.65060f },
  {   0.37490f,   0.00225f,   0.65188f },
  {   0.38105f,   0.00181f,   0.65307f },
  {   0.38718f,   0.00143f,   0.65418f },
  {   0.39330f,   0.00111f,   0.65520f },
  {   0.39941f,   0.00086f,   0.65613f },
  {   0.40550f,   0.00068f,   0.65698f },
  {   0.41158f,   0.00058f,   0.65773f },
  {   0.41764f,   0.00056f,   0.65839f },
  {   0.42369f,   0.00065f,   0.65896f },
  {   0.42972f,   0.00083f,   0.65943f },
  {   0.43573f,   0.00113f,   0.65980f },
  {   0.44173f,   0.00154f,   0.66007f },
  {   0.44771f,   0.00208f,   0.66024f },
  {   0.45368f,   0.00276f,   0.66031f },
  {   0.45962f,   0.00357f,   0.66028f },
  {   0.46555f,   0.00455f,   0.66014f },
  {   0.47146f,   0.00568f,   0.65990f },
  {   0.47734f,   0.00698f,   0.65955f },
  {   0.48321f,   0.00846f,   0.65909f },
  {   0.48906f,   0.01013f,   0.65853f },
  {   0.49488f,   0.01199f,   0.65787f },
  {   0.50068f,   0.01405f,   0.65709f },
  {   0.50645f,   0.01633f,   0.65620f },
  {   0.51221f,   0.01883f,   0.65521f },
  {   0.51793f,   0.02156f,   0.65411f },
  {   0.52363f,   0.02453f,   0.65290f },
  {   0.52931f,   0.02775f,   0.65159f },
  {   0.53495f,   0.03122f,   0.65016f },
  {   0.54057f,   0.03495f,   0.64864f },
  {   0.54616f,   0.03895f,   0.64701f },
  {   0.55171f,   0.04314f,   0.64528f },
  {   0.55724f,   0.04733f,   0.64344f },
  {   0.56274f,   0.05155f,   0.64151f },
  {   0.56820f,   0.05578f,   0.63948f },
  {   0.57363f,   0.06003f,   0.63735f },
  {   0.57903f,   0.06430f,   0.63513f },
  {   0.58439f,   0.06858f,   0.63281f },
  {   0.58972f,   0.07288f,   0.63041f },
  {   0.59501f,   0.07719f,   0.62792f },
  {   0.60027f,   0.08152f,   0.62534f },
  {   0.60549f,   0.08585f,   0.62269f },
  {   0.61067f,   0.09020f,   0.61995f },
  {   0.61581f,   0.09456f,   0.61714f },
  {   0.62092f,   0.09893f,   0.61426f },
  {   0.62599f,   0.10331f,   0.61130f },
  {   0.63102f,   0.10770f,   0.60829f },
  {   0.63601f,   0.11209f,   0.60520f },
  {   0.64096f,   0.11649f,   0.60206f },
  {   0.64587f,   0.12090f,   0.59887f },
  {   0.65075f,   0.12531f,   0.59562f },
  {   0.65558f,   0.12973f,   0.59232f },
  {   0.66037f,   0.13414f,   0.58897f },
  {   0.66513f,   0.13857f,   0.58558f },
  {   0.66985f,   0.14299f,   0.58215f },
  {   0.67452f,   0.14742f,   0.57869f },
  {   0.67916f,   0.15185f,   0.57519f },
  {   0.68376f,   0.15628f,   0.57166f },
  {   0.68832f,   0.16071f,   0.56810f },
  {   0.69284f,   0.16514f,   0.56452f },
  {   0.69732f,   0.16957f,   0.56092f },
  {   0.70177f,   0.17400f,   0.55730f },
  {   0.70618f,   0.17844f,   0.55366f },
  {   0.71055f,   0.18287f,   0.55000f },
  {   0.71488f,   0.18730f,   0.54634f },
  {   0.71918f,   0.19173f,   0.54266f },
  {   0.72344f,   0.19616f,   0.53898f },
  {   0.72767f,   0.20059f,   0.53529f },
  {   0.73186f,   0.20501f,   0.53160f },
  {   0.73602f,   0.20944f,   0.52791f },
  {   0.74014f,   0.21386f,   0.52422f },
  {   0.74423f,   0.21829f,   0.52052f },
  {   0.74829f,   0.22271f,   0.51683f },
  {   0.75231f,   0.22713f,   0.51315f },
  {   0.75630f,   0.23156f,   0.50947f },
  {   0.76026f,   0.23598f,   0.50579f },
  {   0.76419f,   0.24040f,   0.50213f },
  {   0.76809f,   0.24482f,   0.49846f },
  {   0.77196f,   0.24924f,   0.49481f },
  {   0.77580f,   0.25366f,   0.49117f },
  {   0.77960f,   0.25808f,   0.48754f },
  {   0.78338f,   0.26250f,   0.48392f },
  {   0.78713f,   0.26692f,   0.48031f },
  {   0.79085f,   0.27135f,   0.47671f },
  {   0.79455f,   0.27577f,   0.47312f },
  {   0.79822f,   0.28020f,   0.46954f },
  {   0.80185f,   0.28463f,   0.46597f },
  {   0.80547f,   0.28906f,   0.46242f },
  {   0.80905f,   0.29349f,   0.45887f },
  {   0.81261f,   0.29793f,   0.45534f },
  {   0.81614f,   0.30237f,   0.45182f },
  {   0.81965f,   0.30681f,   0.44831f },
  {   0.82313f,   0.31126f,   0.44481f },
  {   0.82659f,   0.31571f,   0.44132f },
  {   0.83002f,   0.32017f,   0.43784f },
  {   0.83342f,   0.32464f,   0.43437f },
  {   0.83680f,   0.32910f,   0.43090f },
  {   0.84015f,   0.33358f,   0.42745f },
  {   0.84348f,   0.33806f,   0.42401f },
  {   0.84679f,   0.34255f,   0.42058f },
  {   0.85007f,   0.34705f,   0.41715f },
  {   0.85332f,   0.35155f,   0.41373f },
  {   0.85655f,   0.35607f,   0.41032f },
  {   0.85975f,   0.36059f,   0.40692f },
  {   0.86293f,   0.36512f,   0.40352f },
  {   0.86608f,   0.36966f,   0.40013f },
  {   0.86920f,   0.37421f,   0.39674f },
  {   0.87230f,   0.37877f,   0.39336f },
  {   0.87538f,   0.38335f,   0.38998f },
  {   0.87842f,   0.38793f,   0.38660f },
  {   0.88144f,   0.39253f,   0.38323f },
  {   0.88444f,   0.39714f,   0.37986f },
  {   0.88740f,   0.40176f,   0.37649f },
  {   0.89034f,   0.40640f,   0.37313f },
  {   0.89325f,   0.41105f,   0.36977f },
  {   0.89613f,   0.41571f,   0.36641f },
  {   0.89898f,   0.42039f,   0.36305f },
  {   0.90181f,   0.42509f,   0.35969f },
  {   0.90460f,   0.42980f,   0.35633f },
  {   0.90736f,   0.43452f,   0.35297f },
  {   0.91010f,   0.43927f,   0.34961f },
  {   0.91280f,   0.44403f,   0.34625f },
  {   0.91547f,   0.44881f,   0.34289f },
  {   0.91811f,   0.45360f,   0.33953f },
  {   0.92071f,   0.45842f,   0.33617f },
  {   0.92329f,   0.46325f,   0.33280f },
  {   0.92583f,   0.46810f,   0.32943f },
  {   0.92833f,   0.47297f,   0.32607f },
  {   0.93080f,   0.47787f,   0.32270f },
  {   0.93323f,   0.48278f,   0.31933f },
  {   0.93563f,   0.48771f,   0.31595f },
  {   0.93799f,   0.49267f,   0.31257f },
  {   0.94031f,   0.49764f,   0.30920f },
  {   0.94260f,   0.50264f,   0.30582f },
  {   0.94484f,   0.50766f,   0.30243f },
  {   0.94705f,   0.51270f,   0.29905f },
  {   0.94922f,   0.51776f,   0.29566f },
  {   0.95134f,   0.52285f,   0.29228f },
  {   0.95343f,   0.52796f,   0.28888f },
  {   0.95547f,   0.53309f,   0.28549f },
  {   0.95747f,   0.53825f,   0.28210f },
  {   0.95942f,   0.54343f,   0.27870f },
  {   0.96134f,   0.54864f,   0.27531f },
  {   0.96320f,   0.55387f,   0.27191f },
  {   0.96502f,   0.55912f,   0.26851f },
  {   0.96680f,   0.56440f,   0.26512f },
  {   0.96853f,   0.56970f,   0.26172f },
  {   0.97020f,   0.57503f,   0.25833f },
  {   0.97184f,   0.58038f,   0.25493f },
  {   0.97342f,   0.58576f,   0.25154f },
  {   0.97495f,   0.59117f,   0.24815f },
  {   0.97643f,   0.59659f,   0.24477f },
  {   0.97786f,   0.60205f,   0.24139f },
  {   0.97923f,   0.60753f,   0.23801f },
  {   0.98056f,   0.61304f,   0.23465f },
  {   0.98183f,   0.61857f,   0.23129f },
  {   0.98304f,   0.62413f,   0.22794f },
  {   0.98420f,   0.62972f,   0.22459f },
  {   0.98530f,   0.63533f,   0.22126f },
  {   0.98635f,   0.64097f,   0.21795f },
  {   0.98733f,   0.64663f,   0.21465f },
  {   0.98826f,   0.65233f,   0.21136f },
  {   0.98913f,   0.65804f,   0.20810f },
  {   0.98994f,   0.66379f,   0.20486f },
  {   0.99068f,   0.66956f,   0.20164f },
  {   0.99137f,   0.67536f,   0.19845f },
  {   0.99199f,   0.68118f,   0.19529f },
  {   0.99254f,   0.68703f,   0.19217f },
  {   0.99303f,   0.69291f,   0.18908f },
  {   0.99346f,   0.69881f,   0.18604f },
  {   0.99381f,   0.70474f,   0.18304f },
  {   0.99410f,   0.71070f,   0.18010f },
  {   0.99432f,   0.71668f,   0.17721f },
  {   0.99447f,   0.72269f,   0.17438f },
  {   0.99455f,   0.72873f,   0.17162f },
  {   0.99456f,   0.73479f,   0.16894f },
  {   0.99450f,   0.74088f,   0.16634f },
  {   0.99435f,   0.74699f,   0.16382f },
  {   0.99414f,   0.75314f,   0.16140f },
  {   0.99385f,   0.75930f,   0.15909f },
  {   0.99348f,   0.76550f,   0.15689f },
  {   0.99303f,   0.77172f,   0.15481f },
  {   0.99250f,   0.77797f,   0.15285f },
  {   0.99190f,   0.78424f,   0.15104f },
  {   0.99121f,   0.79054f,   0.14938f },
  {   0.99044f,   0.79686f,   0.14787f },
  {   0.98959f,   0.80320f,   0.14653f },
  {   0.98865f,   0.80958f,   0.14536f },
  {   0.98762f,   0.81598f,   0.14436f },
  {   0.98651f,   0.82240f,   0.14356f },
  {   0.98531f,   0.82885f,   0.14294f },
  {   0.98403f,   0.83532f,   0.14253f },
  {   0.98265f,   0.84181f,   0.14230f },
  {   0.98119f,   0.84833f,   0.14228f },
  {   0.97964f,   0.85487f,   0.14245f },
  {   0.97799f,   0.86143f,   0.14281f },
  {   0.97627f,   0.86802f,   0.14335f },
  {   0.97444f,   0.87462f,   0.14406f },
  {   0.97253f,   0.88125f,   0.14492f },
  {   0.97053f,   0.88790f,   0.14592f },
  {   0.96844f,   0.89456f,   0.14701f },
  {   0.96627f,   0.90125f,   0.14818f },
  {   0.96402f,   0.90795f,   0.14937f },
  {   0.96168f,   0.91467f,   0.15052f },
  {   0.95928f,   0.92141f,   0.15157f },
  {   0.95681f,   0.92815f,   0.15241f },
  {   0.95429f,   0.93491f,   0.15292f },
  {   0.95173f,   0.94167f,   0.15293f },
  {   0.94915f,   0.94844f,   0.15218f },
  {   0.94660f,   0.95519f,   0.15033f },
  {   0.94415f,   0.96192f,   0.14686f },
  {   0.94190f,   0.96859f,   0.14096f },
  {   0.94002f,   0.97516f,   0.13133f },
};
// clang-format on

} // namespace goom::utils::colordata
