#include <string>
#include <sstream>

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Application.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Math/Color.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Math/BoundingBox.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/VertexBuffer.h>

using namespace Urho3D;
/**
* Using the convenient Application API we don't have
* to worry about initializing the engine or writing a main.
* You can probably mess around with initializing the engine
* and running a main manually, but this is convenient and portable.
*/
class MyApp : public Application
{
public:
    int framecount_;
    float time_;
    SharedPtr<Text> text_;
    SharedPtr<Scene> scene_;
    SharedPtr<Node> boxNode_;
    SharedPtr<Node> cameraNode_;
    SharedPtr<Octree> octree_;

    MyApp(Context * context) : Application(context),framecount_(0),time_(0)
    {
    }

    virtual void Setup()
    {
        // These parameters should be self-explanatory.
        // See http://urho3d.github.io/documentation/1.5/_main_loop.html
        // for a more complete list.
        engineParameters_["FullScreen"]=false;
        engineParameters_["WindowWidth"]=800;
        engineParameters_["WindowHeight"]=600;
        engineParameters_["WindowResizable"]=true;
    }

    virtual void Start()
    {
        ResourceCache* cache=GetSubsystem<ResourceCache>();

        // Let's setup a scene to render.
        scene_=new Scene(context_);
        // Let the scene have an Octree component!
        octree_ = scene_->CreateComponent<Octree>();
        // Let's add an additional scene component for fun.
        scene_->CreateComponent<DebugRenderer>();

        // We need a camera from which the viewport can render.
        cameraNode_=scene_->CreateChild("Camera");
        cameraNode_->SetPosition(Vector3(20,20,20));
        Camera* camera=cameraNode_->CreateComponent<Camera>();
        camera->SetFarClip(2000);

    	Node* zoneNode = scene_->CreateChild("Zone");
    	Zone* zone = zoneNode->CreateComponent<Zone>();
    	zone->SetFogColor(Color(0.1,0.1,0.1));
    	zone->SetFogStart(-5000.0f);
    	zone->SetFogEnd(5000.0f);
    	zone->SetBoundingBox(BoundingBox(-5000.0f,5000.0f));

        // Create a red directional light (sun)
        {
            Node* lightNode=scene_->CreateChild();
            lightNode->SetDirection(Vector3::BACK);
            lightNode->Yaw(50);     // horizontal
            lightNode->Pitch(10);   // vertical
            Light* light=lightNode->CreateComponent<Light>();
            light->SetLightType(LIGHT_DIRECTIONAL);
            light->SetBrightness(1.6);
            light->SetColor(Color(1.0,.6,0.3,1));
            light->SetCastShadows(true);
        }

        // Now we setup the viewport. Of course, you can have more than one!
        Renderer* renderer=GetSubsystem<Renderer>();
        SharedPtr<Viewport> viewport(new Viewport(context_,scene_,cameraNode_->GetComponent<Camera>()));
        renderer->SetViewport(0,viewport);

        SubscribeToEvent(E_KEYDOWN,URHO3D_HANDLER(MyApp,HandleKeyDown));
        SubscribeToEvent(E_UPDATE,URHO3D_HANDLER(MyApp,HandleUpdate));

        {
        	Node* node = scene_->CreateChild();
        	node->SetPosition(Vector3(-10.0f, 5.0f, 0.0f));
        	StaticModel* model = node->CreateComponent<StaticModel>();
        	model->SetModel(cache->GetResource<Model>("Models/TestModel.mdl"));
        	model->SetMaterial(cache->GetResource<Material>("Materials/Material.xml"));
        }
        {
        	Node* node = scene_->CreateChild();
        	node->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
        	AnimatedModel* model = node->CreateComponent<AnimatedModel>();
        	model->SetModel(cache->GetResource<Model>("Models/TestModel.mdl"));
        	model->SetMaterial(cache->GetResource<Material>("Materials/Material.xml"));
        	AnimationController* ac = node->CreateComponent<AnimationController>();
        	ac->Play("Models/Armature.ani", 0, true);
        }
    }

    virtual void Stop()
    {
    }

    void HandleKeyDown(StringHash eventType,VariantMap& eventData)
    {
        using namespace KeyDown;
        int key=eventData[P_KEY].GetInt();
        if(key==KEY_ESCAPE)
            engine_->Exit();

        if(key==KEY_TAB)    // toggle mouse cursor when pressing tab
        {
            GetSubsystem<Input>()->SetMouseVisible(!GetSubsystem<Input>()->IsMouseVisible());
            GetSubsystem<Input>()->SetMouseGrabbed(!GetSubsystem<Input>()->IsMouseGrabbed());
        }
    }

    /**
    * You can get these events from when ever the user interacts with the UI.
    */
    void HandleClosePressed(StringHash eventType,VariantMap& eventData)
    {
        engine_->Exit();
    }

    void HandleUpdate(StringHash eventType,VariantMap& eventData)
    {
        float timeStep=eventData[Update::P_TIMESTEP].GetFloat();
        framecount_++;
        time_+=timeStep;
        // Movement speed as world units per second
        float MOVE_SPEED=20.0f;
        // Mouse sensitivity as degrees per pixel
        const float MOUSE_SENSITIVITY=0.1f;

        Input* input=GetSubsystem<Input>();
        if(input->GetQualifierDown(1))  // 1 is shift, 2 is ctrl, 4 is alt
            MOVE_SPEED*=10;
        if(input->GetKeyDown('W'))
            cameraNode_->Translate(Vector3(0,0, 1)*MOVE_SPEED*timeStep);
        if(input->GetKeyDown('S'))
            cameraNode_->Translate(Vector3(0,0,-1)*MOVE_SPEED*timeStep);
        if(input->GetKeyDown('A'))
            cameraNode_->Translate(Vector3(-1,0,0)*MOVE_SPEED*timeStep);
        if(input->GetKeyDown('D'))
            cameraNode_->Translate(Vector3( 1,0,0)*MOVE_SPEED*timeStep);

        if(!GetSubsystem<Input>()->IsMouseVisible())
        {
            // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
            IntVector2 mouseMove=input->GetMouseMove();
            static float yaw_=-120;
            static float pitch_=20;
            yaw_+=MOUSE_SENSITIVITY*mouseMove.x_;
            pitch_+=MOUSE_SENSITIVITY*mouseMove.y_;
            pitch_=Clamp(pitch_,-90.0f,90.0f);
            // Reset rotation and set yaw and pitch again
            cameraNode_->SetDirection(Vector3::FORWARD);
            cameraNode_->Yaw(yaw_);
            cameraNode_->Pitch(pitch_);
        }

        DebugRenderer* dr = scene_->GetComponent<DebugRenderer>();

		for(float x=-20;x<15;x+=0.2f)
		{
			Drawable* hitDrawable = NULL;
			Vector3 from(x,5.0f,-20.0f);
			Vector3 to(x,5.0f,20.0f);
			Ray ray(from, (to-from).Normalized());
		    PODVector<RayQueryResult> results;
		    RayOctreeQuery query(results, ray, RAY_TRIANGLE, M_INFINITY, DRAWABLE_GEOMETRY);
			octree_->Raycast(query);
			Color lineColor=Color::YELLOW;
			if (results.Size())
			{
				lineColor=Color::RED;
				for(int i=0;i<results.Size();i++)
				{
					if(results.At(i).drawable_->GetTypeName()!="AnimatedModel")
						break;
					if(!TestRaycastOnNodeWithAnimatedModel(ray,results.At(i).node_))
					{
						lineColor=Color::GRAY;
						break;
					}
				}
			}
			dr->AddLine(from, to, lineColor, true);
		}
    }

    bool TestRaycastOnNodeWithAnimatedModel(Ray ray, Node* node)
    {
    	AnimatedModel* animatedModel = node->GetComponent<AnimatedModel>();
    	Model* model = animatedModel->GetModel();
    	Skeleton& skeletion = animatedModel->GetSkeleton();
        Ray localRay=ray;
        localRay.origin_-=node->GetWorldPosition();
        Vector3 outNormal;

        if(skeletion.GetNumBones()==0)
        	return true;

        Vector<Matrix3x4> matricesOfBones;
        matricesOfBones.Reserve(skeletion.GetNumBones());
        for(int s=0;s<skeletion.GetNumBones();s++)
        {
        	Bone* bone = skeletion.GetBone(s);
        	Node* boneNode = node->GetChild(bone->name_,true);
        	Matrix3x4 tarnsformOfBone = boneNode->GetWorldTransform();
        	Quaternion rot;
        	Vector3 pos,scale;
        	tarnsformOfBone.Decompose(pos,rot,scale);
        	pos=pos-node->GetWorldPosition();
        	tarnsformOfBone=Matrix3x4(pos,rot,scale);
        	matricesOfBones.Push(tarnsformOfBone*bone->offsetMatrix_);
        }

        int numberOfBones = matricesOfBones.Size();

        for(int g=0;g<model->GetNumGeometries();g++)
        {
        	Geometry* geometry = model->GetGeometry(g,0);

        	if (geometry)
    		{
    		    const unsigned char* dataOfVertices;
    		    const unsigned char* dataOnIndexes;
    		    unsigned sizeOfVertices;
    		    unsigned sizeOfIndexes;
    		    const PODVector<VertexElement>* elements;

    		    geometry->GetRawData(dataOfVertices, sizeOfVertices, dataOnIndexes, sizeOfIndexes, elements);

    		    unsigned char* dataOfVerticesAnimated = new unsigned char[sizeOfVertices*geometry->GetVertexCount()];
    		    memcpy(dataOfVerticesAnimated,dataOfVertices,sizeOfVertices*geometry->GetVertexCount());

    		    unsigned weightsOffset = VertexBuffer::GetElementOffset(*elements, TYPE_VECTOR4, SEM_BLENDWEIGHTS);
    		    unsigned indicesOffset = VertexBuffer::GetElementOffset(*elements, TYPE_UBYTE4, SEM_BLENDINDICES);
    		    int vc=geometry->GetVertexCount();
    		    for(int v=0;v<vc;v++)
    		    {
    		    	float* ptrPos = (float*) (dataOfVerticesAnimated+v*sizeOfVertices);
    		    	float* ptrWeights = (float*) (dataOfVerticesAnimated+v*sizeOfVertices+weightsOffset);
    		    	unsigned char* ptrIndices = (unsigned char*) (dataOfVerticesAnimated+v*sizeOfVertices+indicesOffset);
    		    	Vector3 baseVertex(ptrPos);
    		    	Vector3 animatedVertex;
    		    	for(int i=0;i<4;i++)
    		    	{
    		    		float w=ptrWeights[i];
    		    		unsigned char ind=ptrIndices[i];
    		    		if(ind<0 || ind>=numberOfBones)
    		    			continue;
    		    		if(w<=0.0f || w>1.0f)
    		    			continue;
    		    		animatedVertex=animatedVertex+matricesOfBones[ind]*w*(baseVertex);
    		    	}
    		    	ptrPos[0]=animatedVertex.x_;
    		    	ptrPos[1]=animatedVertex.y_;
    		    	ptrPos[2]=animatedVertex.z_;
    		    }

    			float distanceToGeometry = localRay.HitDistance(dataOfVerticesAnimated, sizeOfVertices, dataOnIndexes, sizeOfIndexes,
    					geometry->GetIndexStart(), geometry->GetIndexCount(), &outNormal, NULL, 0);

    			delete [] dataOfVerticesAnimated;

    			if (distanceToGeometry < M_INFINITY)
    		    	return true;
    		}
        }

    	return false;
    }
};

URHO3D_DEFINE_APPLICATION_MAIN(MyApp)
