// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "bezierNi.h"
#include "bezier_hair_intersector.h"
#include "bezier_ribbon_intersector.h"
#include "bezier_curve_intersector.h"

namespace embree
{
  namespace isa
  {
    struct BezierNiIntersector1
    {
      typedef BezierNi Primitive;

      struct Precalculations
      {
        __forceinline Precalculations() {}

        __forceinline Precalculations(const Ray& ray, const void* ptr)
          : intersectorHair(ray,ptr), intersectorCurve(ray,ptr) {}

        Bezier1Intersector1<Curve3fa> intersectorHair;
        BezierCurve1Intersector1<Curve3fa> intersectorCurve;
      };

      static __forceinline void intersect(const Precalculations& pre, RayHit& ray, IntersectContext* context, const Primitive& prim)
      {
        const Vec3fa org1 = xfmPoint (prim.space,ray.org);
        const Vec3fa dir1 = xfmVector(prim.space,ray.dir);
        const Vec3fa rcp_dir1 = rcp(dir1);
        
        const vfloat8 t_lower_x = (vfloat8::load(prim.items.lower_x)-vfloat8(org1.x))*vfloat8(rcp_dir1.x);
        const vfloat8 t_upper_x = (vfloat8::load(prim.items.upper_x)-vfloat8(org1.x))*vfloat8(rcp_dir1.x);
        const vfloat8 t_lower_y = (vfloat8::load(prim.items.lower_y)-vfloat8(org1.y))*vfloat8(rcp_dir1.y);
        const vfloat8 t_upper_y = (vfloat8::load(prim.items.upper_y)-vfloat8(org1.y))*vfloat8(rcp_dir1.y);
        const vfloat8 t_lower_z = (vfloat8::load(prim.items.lower_z)-vfloat8(org1.z))*vfloat8(rcp_dir1.z);
        const vfloat8 t_upper_z = (vfloat8::load(prim.items.upper_z)-vfloat8(org1.z))*vfloat8(rcp_dir1.z);

        const vfloat8 tNear = max(min(t_lower_x,t_upper_x),min(t_lower_y,t_upper_y),min(t_lower_z,t_upper_z),vfloat8(ray.tnear()));
        const vfloat8 tFar  = min(max(t_lower_x,t_upper_x),max(t_lower_y,t_upper_y),max(t_lower_z,t_upper_z),vfloat8(ray.tfar));
        vbool8 valid = (vint8(step) < vint8(prim.N)) & (tNear <= tFar);

        while (any(valid))
        {
          size_t i = select_min(valid,tNear);
          clear(valid,i);
        
          STAT3(normal.trav_prims,1,1,1);
          const unsigned int geomID = prim.items.geomID[i];
          const unsigned int primID = prim.items.primID[i];
          const NativeCurves* geom = (NativeCurves*) context->scene->get(geomID);
          Vec3fa a0,a1,a2,a3; geom->gather(a0,a1,a2,a3,geom->curve(primID));
          if (likely(geom->subtype == FLAT_CURVE))
            pre.intersectorHair.intersect(ray,a0,a1,a2,a3,geom->tessellationRate,Intersect1EpilogMU<VSIZEX,true>(ray,context,geomID,primID));
          else 
            pre.intersectorCurve.intersect(ray,a0,a1,a2,a3,Intersect1Epilog1<true>(ray,context,geomID,primID));
        }
      }
      
      static __forceinline bool occluded(const Precalculations& pre, Ray& ray, IntersectContext* context, const Primitive& prim) {
        return false;
      }
    };
  }
}
