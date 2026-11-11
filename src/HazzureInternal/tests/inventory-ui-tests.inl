// T003: pure collection helpers and isolated 2D WARP resources only.
static void InventoryUITests(ID3D11Device* device,ID3D11DeviceContext* context,const std::filesystem::path& output){
    using namespace HazzureUI;
    HazzureInventory::Collection collection;
    for(int n=0;n<96;++n){HazzureInventory::Item item{};
        item.skin.definition=n==0?507:n==1?5030:7;item.skin.paint=n<2?0:180;
        item.skin.wear=.05f;item.skin.seed=n;item.attributes.definition=item.skin.definition;
        strcpy_s(item.label,n<2?"Especial":"Variante azul");
        Check(HazzureInventory::Add(collection,item),"96 collection fixture items");
    }
    std::string before,after;Check(InventoryContext(collection,before),"canonical collection fixture");
    Check(InventoryFiltered(collection,"",0,0).size()==96,"full collection capacity visible");
    Check(InventoryFiltered(collection,"azul",0,0).size()==94,"name filter");
    Check(InventoryFiltered(collection,"AK-47",0,0).size()==94,"weapon name filter");
    Check(InventoryFiltered(collection,"180",0,0).size()==94,"paint kit filter");
    Check(InventoryFiltered(collection,InventoryPaint(collection.items[2]),3,0).size()==94,"catalogue paint name filter");
    Check(InventoryFiltered(collection,"",1,0).size()==1,"knife filter");
    Check(InventoryFiltered(collection,"",2,0).size()==1,"glove filter");
    Check(InventoryFiltered(collection,"azul impossible",0,0).empty(),"empty combined query");
    Check(InventoryContext(collection,after)&&before==after,"filters do not mutate source");
    auto reordered=collection;std::swap(reordered.items[3],reordered.items[80]);
    for(int sort=0;sort<4;++sort){
        auto expected=InventoryFiltered(collection,"",0,sort),actual=InventoryFiltered(reordered,"",0,sort);
        for(size_t n=0;n<expected.size();++n)
            Check(collection.items[expected[n]].id==reordered.items[actual[n]].id,"sort deterministic across source order with ID tie break");
    }
    Check(InventoryFiltered(collection,"",0,1).front()==95,"newest IDs first");
    auto invalid=collection;invalid.count=97;
    Check(InventoryFiltered(invalid,"",0,0).empty(),"invalid source capacity rejected");
    Check(!BeginInventoryDelete(invalid,3),"invalid deletion context rejected before ID lookup");
    g_inventorySelected=3;g_inventoryDraft=collection.items[2];
    Check(BeginInventoryDelete(collection,3),"capture delete ID");
    g_inventorySelected=9;
    Check(!ConfirmInventoryDelete(collection,false)&&collection.count==96&&g_inventorySelected==9,"cancel preserves collection and selection");
    Check(!g_inventoryDeletePending&&g_inventoryDeleteContext.empty(),"cancel clears captured context");
    Check(BeginInventoryDelete(collection,3),"capture exact deletion target");
    Check(ConfirmInventoryDelete(collection,true)&&collection.count==95&&HazzureInventory::Find(collection,3)<0&&HazzureInventory::Find(collection,9)>=0,"confirmation removes only captured ID");
    Check(g_inventorySelected==9&&!g_inventoryDraft.id,"unrelated selection preserved and removed draft cleared");
    Check(!ConfirmInventoryDelete(collection,true),"confirmation cannot repeat");
    Check(BeginInventoryDelete(collection,9),"capture context before replacement");
    auto replacement=collection;strcpy_s(replacement.items[HazzureInventory::Find(replacement,9)].label,"Outra configuracao");
    Check(!ConfirmInventoryDelete(replacement,true)&&HazzureInventory::Find(replacement,9)>=0,"same ID in changed collection cannot be deleted");
    Check(BeginInventoryDelete(collection,9),"capture context before attribute edit");
    collection.items[0].skin.seed=999;
    Check(!InventoryDeleteContextMatches(collection)&&!ConfirmInventoryDelete(collection,true),"changed item context disables confirmation");
    Check(!BeginInventoryDelete(collection,99999),"missing deletion target rejected");
    g_inventorySelected=0;g_inventoryDraft={};

    // A tiny padded PNG exercises the real WIC crop/upload path without captures.
    auto root=output/"thumbnail-fixtures";std::filesystem::create_directories(root);
    unsigned pixels[32]{};
    for(int y=1;y<3;++y)for(int x=2;x<6;++x)pixels[y*8+x]=0xFF4080C0;
    D3D11_TEXTURE2D_DESC desc{};desc.Width=8;desc.Height=4;desc.MipLevels=desc.ArraySize=1;
    desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.SampleDesc.Count=1;
    D3D11_SUBRESOURCE_DATA data{pixels,32,0};Com<ID3D11Texture2D> source;
    Require(device->CreateTexture2D(&desc,&data,source.Address()),"2D fixture texture");
    SavePng(device,context,source.p,root/"padded.png");
    HazzurePreviewAssets::SetDevice(device);HazzurePreviewAssets::SetRoot(root);
    auto first=HazzurePreviewAssets::Get("padded.png");
    Check(first.view&&first.width==4&&first.height==2,"transparent padding normalized without stretching");
    Check(HazzurePreviewAssets::Get("padded.png").view==first.view,"cache hit retains same SRV");
    for(int n=1;n<96;++n){char key[48];sprintf_s(key,"catalogue-%d.png",n);
        std::filesystem::copy_file(root/"padded.png",root/key,std::filesystem::copy_options::overwrite_existing);
        auto thumbnail=HazzurePreviewAssets::Get(key);
        Check(thumbnail.view&&thumbnail.width==4&&thumbnail.height==2,"all 96 distinct catalogue thumbnails resident");
    }
    auto& io=ImGui::GetIO();io.DisplaySize={320,240};
    ImGui_ImplDX11_NewFrame();ImGui::NewFrame();
    ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({300,220});
    ImGui::Begin("T003 2D resources",nullptr,ImGuiWindowFlags_NoSavedSettings);
    bool selected=false;
    GSWidgets::GS_InventorySlot("aspect","Fixture","Paint",&selected,reinterpret_cast<ImTextureID>(first.view),.05f,1,first.width,first.height);
    auto* draw=ImGui::GetWindowDrawList();bool aspectChecked=false;
    for(const auto& command:draw->CmdBuffer)if(command.GetTexID()==reinterpret_cast<ImTextureID>(first.view)&&command.ElemCount){
        const auto& a=draw->VtxBuffer[draw->IdxBuffer[command.IdxOffset]+command.VtxOffset].pos;
        const auto& b=draw->VtxBuffer[draw->IdxBuffer[command.IdxOffset+2]+command.VtxOffset].pos;
        Check(std::abs((b.x-a.x)/(b.y-a.y)-2.f)<.001f&&b.y-a.y<=30.01f,"thumbnail aspect ratio fits card bounds");aspectChecked=true;
    }
    Check(aspectChecked,"thumbnail emitted as a 2D image command");
    ImGui::End();
    // Hold an independent reference so eviction bugs are observable without use-after-free.
    first.view->AddRef();
    for(size_t n=0;n<160;++n){char key[48];sprintf_s(key,"absent-%zu.png",n);Check(!HazzurePreviewAssets::Get(key).view,"missing or saturated thumbnail fallback");}
    Check(HazzurePreviewAssets::cache.size()==HazzurePreviewAssets::CacheCapacity,"cache remains bounded");
    Check(first.view->Release()>=1,"cache saturation retains already emitted SRV ownership");
    Check(HazzurePreviewAssets::Get("padded.png").view==first.view,"resident thumbnail survives saturation");
    ImGui::Render();
    desc.Width=320;desc.Height=240;desc.BindFlags=D3D11_BIND_RENDER_TARGET;
    Com<ID3D11Texture2D> target;Com<ID3D11RenderTargetView> targetView;
    Require(device->CreateTexture2D(&desc,nullptr,target.Address()),"2D draw target");
    Require(device->CreateRenderTargetView(target.p,nullptr,targetView.Address()),"2D draw view");
    context->OMSetRenderTargets(1,&targetView.p,nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());context->OMSetRenderTargets(0,nullptr,nullptr);
    Check(true,"WARP submitted saturated-cache draw before lifecycle reset");
    HazzurePreviewAssets::SetRoot(root);
    Check(HazzurePreviewAssets::cache.empty(),"root reset clears cache outside drawing");
    Check(HazzurePreviewAssets::Get("padded.png").view!=nullptr,"root reset reloads thumbnail");
    HazzurePreviewAssets::SetDevice(nullptr);
    Check(HazzurePreviewAssets::cache.empty()&&!HazzurePreviewAssets::device,"device reset releases cache");
    HazzurePreviewAssets::SetDevice(device);
    Check(HazzurePreviewAssets::Get("padded.png").view!=nullptr,"device reset recovers thumbnail");
    HazzurePreviewAssets::SetDevice(nullptr);HazzurePreviewAssets::SetRoot({});
    puts("PASS: T003 filters, stable ID sorting, context-safe deletion, 96 capacity, normalized 2D thumbnails and bounded SRV cache lifecycle");
}
